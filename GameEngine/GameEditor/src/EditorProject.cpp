#include "EditorProject.h"

#include <fstream>
#include <unordered_set>
#include <algorithm>
#include <cwctype>

std::vector<FileEntry> g_files;
int g_selectedIndex = -1;
fs::path g_projectRoot = fs::path("EngineFiles");
static std::unordered_set<std::wstring> g_expanded;
static fs::path g_clipboardPath;
static bool g_clipboardHasData = false;

enum class OpType
{
    CreatePath,
    CopyPath,
    DeletePath,
    RenamePath,
    MovePath
};

struct UndoOp
{
    OpType type{};
    fs::path src;
    fs::path dst;
};

static std::vector<UndoOp> g_undo;
static std::vector<UndoOp> g_redo;

static std::wstring ToKey(const fs::path& p)
{
    return p.lexically_normal().wstring();
}

static bool DeletePath(const fs::path& path)
{
    if (!fs::exists(path)) return true;
    return fs::remove_all(path) > 0;
}

static bool IsSubPath(const fs::path& parent, const fs::path& child);
static void WriteAssetMeta(const fs::path& assetPath, const char* type);

static bool IsParentSelected(const fs::path& path, const std::vector<fs::path>& paths)
{
    for (const auto& p : paths)
    {
        if (p == path) continue;
        if (IsSubPath(p, path))
            return true;
    }
    return false;
}

static fs::path MakeUniquePath(const fs::path& base)
{
    if (!fs::exists(base)) return base;
    fs::path parent = base.parent_path();
    std::wstring stem = base.stem().wstring();
    std::wstring ext = base.extension().wstring();

    for (int i = 1; i < 1000; ++i)
    {
        std::wstring name = stem + L"_copy";
        if (i > 1) name += std::to_wstring(i);
        fs::path candidate = parent / (name + ext);
        if (!fs::exists(candidate)) return candidate;
    }
    return base;
}

static bool CopyPathTo(const fs::path& src, const fs::path& dst)
{
    if (!fs::exists(src)) return false;
    if (fs::is_directory(src))
    {
        fs::create_directories(dst);
        fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::skip_existing);
        return true;
    }
    fs::copy_file(src, dst, fs::copy_options::skip_existing);
    return true;
}

static bool IsSubPath(const fs::path& parent, const fs::path& child)
{
    auto p = parent.lexically_normal();
    auto c = child.lexically_normal();
    auto pIt = p.begin();
    auto cIt = c.begin();
    for (; pIt != p.end() && cIt != c.end(); ++pIt, ++cIt)
    {
        if (*pIt != *cIt) return false;
    }
    return pIt == p.end();
}

static fs::path GetTrashRoot()
{
    if (g_projectRoot.empty()) return fs::path();
    fs::path trash = g_projectRoot / L".editor_trash";
    fs::create_directories(trash);
    return trash;
}

static bool IsInternalPath(const fs::path& path)
{
    auto name = path.filename().wstring();
    return name == L".editor_trash" || name == L"editor_state.json";
}

static fs::path MakeTrashPath(const fs::path& src)
{
    fs::path trashRoot = GetTrashRoot();
    if (trashRoot.empty()) return fs::path();
    fs::path dest = trashRoot / src.filename();
    return MakeUniquePath(dest);
}

void CollectTree(const fs::path& root, int depth, std::vector<FileEntry>& out)
{
    if (!fs::exists(root)) return;
    std::vector<fs::directory_entry> entries;
    for (auto& p : fs::directory_iterator(root))
    {
        if (IsInternalPath(p.path()))
            continue;
        entries.push_back(p);
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b)
    {
        if (a.is_directory() != b.is_directory())
            return a.is_directory();
        return a.path().filename().wstring() < b.path().filename().wstring();
    });

    for (auto& p : entries)
    {
        FileEntry e;
        e.depth = depth;
        e.isDir = p.is_directory();
        e.path = p.path();
        e.label = p.path().filename().wstring();
        out.push_back(e);

        if (e.isDir && IsExpanded(e.path))
            CollectTree(p.path(), depth + 1, out);
    }
}

void RefreshFiles()
{
    g_files.clear();
    if (!g_projectRoot.empty())
    {
        CollectTree(g_projectRoot, 0, g_files);
    }
    if (g_selectedIndex >= (int)g_files.size())
        g_selectedIndex = (int)g_files.size() - 1;
    if (g_selectedIndex < 0 && !g_files.empty())
        g_selectedIndex = 0;
}

fs::path GetSelectedBase()
{
    fs::path base = g_projectRoot;
    if (g_selectedIndex >= 0 && g_selectedIndex < (int)g_files.size())
    {
        if (g_files[g_selectedIndex].isDir)
            base = g_files[g_selectedIndex].path;
        else
            base = g_files[g_selectedIndex].path.parent_path();
    }
    return base;
}

void SetProjectRoot(const fs::path& root)
{
    g_projectRoot = root;
    g_expanded.clear();
    if (!g_projectRoot.empty())
        g_expanded.insert(ToKey(g_projectRoot));
    EnsureProjectLayout();
}

const fs::path& GetProjectRoot()
{
    return g_projectRoot;
}

std::wstring GetProjectName()
{
    if (g_projectRoot.empty()) return L"(none)";
    return g_projectRoot.filename().wstring();
}

bool IsExpanded(const fs::path& path)
{
    return g_expanded.find(ToKey(path)) != g_expanded.end();
}

bool ToggleExpanded(const fs::path& path)
{
    auto key = ToKey(path);
    if (g_expanded.find(key) != g_expanded.end())
    {
        g_expanded.erase(key);
        return false;
    }
    g_expanded.insert(key);
    return true;
}

void CreateNewFolder()
{
    fs::path base = GetSelectedBase();
    fs::path newDir = base / L"NewFolder";
    int suffix = 1;
    while (fs::exists(newDir))
        newDir = base / (L"NewFolder" + std::to_wstring(suffix++));
    fs::create_directories(newDir);
    g_undo.push_back({ OpType::CreatePath, {}, newDir });
    g_redo.clear();
    AddLog(L"[project] created folder " + newDir.filename().wstring());
    RefreshFiles();
}

void CreateNewFile()
{
    fs::path base = GetSelectedBase();
    fs::path file = base / L"NewFile.txt";
    int suffix = 1;
    while (fs::exists(file))
        file = base / (L"NewFile" + std::to_wstring(suffix++) + L".txt");
    std::ofstream out(file);
    out << "// GameEditor\n";
    out.close();
    g_undo.push_back({ OpType::CreatePath, {}, file });
    g_redo.clear();
    AddLog(L"[project] created file " + file.filename().wstring());
    RefreshFiles();
}

void CopySelectionToClipboard()
{
    if (g_selectedIndex < 0 || g_selectedIndex >= (int)g_files.size())
        return;
    g_clipboardPath = g_files[g_selectedIndex].path;
    g_clipboardHasData = true;
    AddLog(L"[project] copied " + g_clipboardPath.filename().wstring());
}

void PasteClipboard()
{
    if (!g_clipboardHasData) return;
    fs::path base = GetSelectedBase();
    fs::path dest = base / g_clipboardPath.filename();
    dest = MakeUniquePath(dest);
    if (CopyPathTo(g_clipboardPath, dest))
    {
        g_undo.push_back({ OpType::CopyPath, g_clipboardPath, dest });
        g_redo.clear();
        AddLog(L"[project] pasted " + dest.filename().wstring());
        RefreshFiles();
    }
}

void DeleteSelected()
{
    if (g_selectedIndex < 0 || g_selectedIndex >= (int)g_files.size())
        return;
    const auto& e = g_files[g_selectedIndex];
    if (IsInternalPath(e.path))
        return;
    fs::path trash = MakeTrashPath(e.path);
    if (trash.empty()) return;

    fs::rename(e.path, trash);
    g_undo.push_back({ OpType::DeletePath, e.path, trash });
    g_redo.clear();
    AddLog(L"[project] deleted " + e.path.filename().wstring());
    RefreshFiles();
}

void DeletePaths(const std::vector<fs::path>& paths)
{
    if (paths.empty()) return;
    std::vector<fs::path> unique;
    unique.reserve(paths.size());
    for (const auto& p : paths)
    {
        if (IsInternalPath(p))
            continue;
        if (!fs::exists(p))
            continue;
        if (IsParentSelected(p, paths))
            continue;
        unique.push_back(p);
    }
    if (unique.empty()) return;

    for (const auto& p : unique)
    {
        fs::path trash = MakeTrashPath(p);
        if (trash.empty()) continue;
        try
        {
            fs::rename(p, trash);
        }
        catch (...)
        {
            continue;
        }
        g_undo.push_back({ OpType::DeletePath, p, trash });
    }
    g_redo.clear();
    AddLog(L"[project] deleted " + std::to_wstring(unique.size()) + L" item(s)");
    RefreshFiles();
}

void RenameSelected(const std::wstring& newName)
{
    if (g_selectedIndex < 0 || g_selectedIndex >= (int)g_files.size())
        return;
    if (newName.empty()) return;

    const auto& e = g_files[g_selectedIndex];
    fs::path dst = e.path.parent_path() / newName;
    if (fs::exists(dst)) return;

    try
    {
        fs::rename(e.path, dst);
    }
    catch (...)
    {
        return;
    }
    g_undo.push_back({ OpType::RenamePath, e.path, dst });
    g_redo.clear();
    AddLog(L"[project] renamed to " + dst.filename().wstring());
    RefreshFiles();
}

void ImportDropped(const std::vector<fs::path>& paths)
{
    if (paths.empty()) return;
    for (const auto& p : paths)
    {
        if (!fs::exists(p)) continue;
        fs::path destBase = GetSelectedBase();
        if (!g_projectRoot.empty() && !fs::is_directory(p))
        {
            std::wstring ext = p.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
            if (ext == L".fbx" || ext == L".obj" || ext == L".blend" || ext == L".gltf" || ext == L".glb")
                destBase = g_projectRoot / L"Assets" / L"Models";
            else if (ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".tga" || ext == L".bmp" || ext == L".dds")
                destBase = g_projectRoot / L"Assets" / L"Textures";
            else if (ext == L".wav" || ext == L".mp3" || ext == L".ogg")
                destBase = g_projectRoot / L"Assets" / L"Audio";
            else if (ext == L".cpp" || ext == L".h" || ext == L".hpp" || ext == L".c" || ext == L".cs")
                destBase = g_projectRoot / L"Scripts";
        }
        fs::path dest = destBase / p.filename();
        dest = MakeUniquePath(dest);
        if (CopyPathTo(p, dest))
        {
            g_undo.push_back({ OpType::CopyPath, p, dest });
            std::wstring ext = dest.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
            if (ext == L".fbx" || ext == L".obj" || ext == L".blend" || ext == L".gltf" || ext == L".glb")
                WriteAssetMeta(dest, "model");
            else if (ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".tga" || ext == L".bmp" || ext == L".dds")
                WriteAssetMeta(dest, "texture");
            else if (ext == L".wav" || ext == L".mp3" || ext == L".ogg")
                WriteAssetMeta(dest, "audio");
        }
    }
    g_redo.clear();
    RefreshFiles();
}

void MoveSelectedTo(const fs::path& targetDir)
{
    if (g_selectedIndex < 0 || g_selectedIndex >= (int)g_files.size())
        return;
    if (targetDir.empty() || !fs::exists(targetDir) || !fs::is_directory(targetDir))
        return;

    const auto& e = g_files[g_selectedIndex];
    fs::path dst = targetDir / e.path.filename();
    if (dst == e.path) return;
    if (e.isDir && IsSubPath(e.path, dst)) return;
    if (fs::exists(dst)) dst = MakeUniquePath(dst);

    try
    {
        fs::rename(e.path, dst);
    }
    catch (...)
    {
        return;
    }
    g_undo.push_back({ OpType::MovePath, e.path, dst });
    g_redo.clear();
    AddLog(L"[project] moved " + e.path.filename().wstring());
    RefreshFiles();
}

void MovePathsTo(const std::vector<fs::path>& paths, const fs::path& targetDir)
{
    if (paths.empty()) return;
    if (targetDir.empty() || !fs::exists(targetDir) || !fs::is_directory(targetDir))
        return;

    std::vector<fs::path> unique;
    unique.reserve(paths.size());
    for (const auto& p : paths)
    {
        if (!fs::exists(p)) continue;
        if (IsInternalPath(p)) continue;
        if (IsParentSelected(p, paths)) continue;
        unique.push_back(p);
    }
    if (unique.empty()) return;

    for (const auto& p : unique)
    {
        fs::path dst = targetDir / p.filename();
        if (dst == p) continue;
        if (fs::exists(dst)) dst = MakeUniquePath(dst);
        if (fs::is_directory(p) && IsSubPath(p, dst)) continue;
        try
        {
            fs::rename(p, dst);
        }
        catch (...)
        {
            continue;
        }
        g_undo.push_back({ OpType::MovePath, p, dst });
    }
    g_redo.clear();
    AddLog(L"[project] moved " + std::to_wstring(unique.size()) + L" item(s)");
    RefreshFiles();
}

void UndoLast()
{
    if (g_undo.empty()) return;
    UndoOp op = g_undo.back();
    g_undo.pop_back();

    if (op.type == OpType::CreatePath)
    {
        DeletePath(op.dst);
    }
    else if (op.type == OpType::CopyPath)
    {
        DeletePath(op.dst);
    }
    else if (op.type == OpType::DeletePath)
    {
        if (fs::exists(op.dst))
            fs::rename(op.dst, op.src);
    }
    else if (op.type == OpType::RenamePath)
    {
        if (fs::exists(op.dst))
            fs::rename(op.dst, op.src);
    }
    else if (op.type == OpType::MovePath)
    {
        if (fs::exists(op.dst))
            fs::rename(op.dst, op.src);
    }

    g_redo.push_back(op);
    RefreshFiles();
}

void RedoLast()
{
    if (g_redo.empty()) return;
    UndoOp op = g_redo.back();
    g_redo.pop_back();

    if (op.type == OpType::CreatePath)
    {
        if (op.dst.extension().empty())
            fs::create_directories(op.dst);
        else
            std::ofstream(op.dst).close();
    }
    else if (op.type == OpType::CopyPath)
    {
        CopyPathTo(op.src, op.dst);
    }
    else if (op.type == OpType::DeletePath)
    {
        if (fs::exists(op.src))
            fs::rename(op.src, op.dst);
    }
    else if (op.type == OpType::RenamePath)
    {
        if (fs::exists(op.src))
            fs::rename(op.src, op.dst);
    }
    else if (op.type == OpType::MovePath)
    {
        if (fs::exists(op.src))
            fs::rename(op.src, op.dst);
    }

    g_undo.push_back(op);
    RefreshFiles();
}

bool CanUndo()
{
    return !g_undo.empty();
}

bool CanRedo()
{
    return !g_redo.empty();
}

void SaveEditorState()
{
    if (g_projectRoot.empty()) return;
    fs::path state = g_projectRoot / L"editor_state.json";
    std::ofstream out(state);
    out << "{\n";
    out << "  \"lastSelection\": " << g_selectedIndex << "\n";
    out << "}\n";
    out.close();
    AddLog(L"[editor] saved editor_state.json");
}

void EnsureProjectLayout()
{
    if (g_projectRoot.empty()) return;
    fs::create_directories(g_projectRoot / L"Assets" / L"Models");
    fs::create_directories(g_projectRoot / L"Assets" / L"Textures");
    fs::create_directories(g_projectRoot / L"Assets" / L"Audio");
    fs::create_directories(g_projectRoot / L"Scripts");
    fs::create_directories(g_projectRoot / L"Scenes");
}

static void WriteAssetMeta(const fs::path& assetPath, const char* type)
{
    if (assetPath.empty()) return;
    fs::path meta = assetPath;
    meta += L".asset.json";
    if (fs::exists(meta)) return;
    std::ofstream out(meta);
    out << "{\n";
    out << "  \"type\": \"" << type << "\",\n";
    if (std::string(type) == "model")
        out << "  \"scale\": 1.0,\n";
    out << "  \"source\": \"" << assetPath.filename().string() << "\"\n";
    out << "}\n";
    out.close();
}
