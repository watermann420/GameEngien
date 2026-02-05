#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include "EditorProject.h"
#include "EditorDraw.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef cos
#undef cos
#endif
#ifdef sin
#undef sin
#endif


static std::vector<std::wstring> g_logs;
static const int kTopBarH = 36;
static HWND g_mainWnd = nullptr;
static HWND g_consoleWnd = nullptr;
static bool g_consoleVisible = false;
static HWND g_settingsWnd = nullptr;
static std::wstring g_codeEditorPath;
static const int kConsoleLinesMax = 200;
enum
{
    ID_TOGGLE_CONSOLE = 1001,
    ID_TOGGLE_PROJECT = 1002,
    ID_TOGGLE_SCENE = 1003,
    ID_TOGGLE_INSPECTOR = 1004,
    ID_VIEW_CLOSE_ALL = 1010,
    ID_VIEW_RESET_LAYOUT = 1011,
    ID_CTX_NEW_FILE = 1101,
    ID_CTX_NEW_FOLDER = 1102,
    ID_CTX_RENAME = 1103,
    ID_CTX_DELETE = 1104,
    ID_CTX_COPY = 1105,
    ID_CTX_PASTE = 1106,
    ID_CTX_HIER_RENAME = 1201,
    ID_CTX_HIER_DELETE = 1202,
    ID_CTX_HIER_NEW_GROUP = 1203,
    ID_SETTINGS_SET_EDITOR = 1301,
    ID_SETTINGS_OPEN_PROJECT = 1302
};
enum { ID_NAV_TIMER = 11 };
enum { ID_ANIM_TIMER = 10 };

static bool g_playing = false;
static ULONGLONG g_lastTick = 0;
static ULONGLONG g_startTick = 0;
static bool g_smokeMode = false;
static int g_smokeSeconds = 6;
static bool g_showProject = true;
static bool g_showConsole = false;
static bool g_showScene = true;
static bool g_showInspector = true;
static int g_bottomH = 220;
static int g_consoleH = 110;
static bool g_projectCollapsed = true;
static int g_prevBottomH = 220;
static bool g_consoleJustOpened = false;
static bool g_dragBottom = false;
static bool g_dragConsole = false;
static int g_dragStartY = 0;
static int g_dragStartH = 0;

static const int kLineH = 16;
static const int kResizeBorder = 6;
static const int kInspectorH = 180;
static const int kPaneHeaderH = 22;
static const int kSplitterH = 4;
static const int kMinBottomH = 140;
static const int kMinConsoleH = 90;
static int g_leftPanelW = 260;
static int g_rightPanelW = 260;
static bool g_showLeftPanel = false;
static bool g_showRightPanel = false;
static float g_sceneZoom = 1.0f;
static int g_scenePanX = 0;
static int g_scenePanY = 0;
static bool g_panDrag = false;
static POINT g_panLast{};
static bool g_dragLeftSplit = false;
static bool g_dragRightSplit = false;
static RECT g_leftSplitRect{};
static RECT g_rightSplitRect{};

static HMENU g_menuFile = nullptr;
static HMENU g_menuEdit = nullptr;
static HMENU g_menuView = nullptr;
static HMENU g_menuWindow = nullptr;
static HMENU g_menuHelp = nullptr;

static std::vector<RECT> g_menuRects;
static int g_hoverMenu = -1;
static RECT g_btnPlay{};
static RECT g_btnNewFile{};
static RECT g_btnNewFolder{};
static RECT g_btnRefresh{};
static RECT g_btnToggleConsole{};
static RECT g_projectListRect{};
static RECT g_bottomSplitterRect{};
static RECT g_consoleSplitterRect{};
static RECT g_consoleHeaderRect{};
static RECT g_projectHeaderRect{};
static RECT g_btnToggleInspector{};
static int g_hoverFileIndex = -1;
static bool g_dragFile = false;
static bool g_dragFileActive = false;
static POINT g_dragStart{};
static POINT g_dragPos{};
static int g_dragHoverIndex = -1;
static bool g_dragSelect = false;
static POINT g_selectStart{};
static RECT g_selectRect{};
static int g_selectTarget = 0; // 0 none, 1 file tree, 2 hierarchy
static int g_fileAnchor = -1;
static std::unordered_set<std::wstring> g_fileSelected;
static bool g_dragHierarchy = false;
static bool g_dragHierarchyActive = false;
static POINT g_hierarchyDragStart{};
static POINT g_hierarchyDragPos{};
static int g_hierarchyDragHoverIndex = -1;
static HWND g_renameEdit = nullptr;
static int g_renameIndex = -1;
static WNDPROC g_oldEditProc = nullptr;
static RECT g_btnLeftCollapse{};
static RECT g_btnRightCollapse{};
static RECT g_leftPanelRect{};
static RECT g_leftListRect{};
static RECT g_sceneBodyRect{};
static RECT g_btnViewMode{};
static int g_hierarchyHover = -1;
static int g_hierarchySelected = -1;
static bool g_hierarchySceneExpanded = true;
static std::wstring g_hierarchyRenameLabel;
static int g_renameTarget = 0; // 0=file, 1=hierarchy
static HBRUSH g_editBg = nullptr;
static std::unordered_map<std::wstring, std::wstring> g_hierarchyRenameMap;
static int g_renameHierIndex = -1;
static std::unordered_set<std::wstring> g_hierarchySelectedSet;
static int g_hierarchyAnchor = -1;
static bool g_view3D = false;
static bool g_mouseLook = false;
static POINT g_mouseLookLast{};
static POINT g_mouseLookCenter{};
static bool g_focusHierarchy = false;
static bool g_skipHierarchyDeleteConfirm = false;
static std::unordered_map<std::wstring, bool> g_hierarchyDeleted;
static std::vector<std::wstring> g_hierarchyUndoStack;
static std::vector<std::wstring> g_hierarchyGroups;
static std::unordered_map<std::wstring, std::wstring> g_hierarchyParent;
static std::unordered_map<std::wstring, bool> g_hierarchyGroupExpanded;
static float g_camYaw = 0.0f;
static float g_camPitch = 0.0f;
static float g_camX = 0.0f;
static float g_camY = 0.0f;
static float g_camZ = -5.0f;

static std::wstring GetEnvVar(const wchar_t* name)
{
    wchar_t* value = nullptr;
    size_t len = 0;
    if (_wdupenv_s(&value, &len, name) != 0 || !value)
        return L"";
    std::wstring out = value;
    free(value);
    return out;
}

static std::wstring GetEditorExecutable()
{
    if (!g_codeEditorPath.empty() && fs::exists(g_codeEditorPath))
        return g_codeEditorPath;
    for (const auto* name : { L"CODE_EDITOR_PATH", L"RIDER_PATH", L"RIDER_EXE", L"RIDER64_EXE" })
    {
        std::wstring candidate = GetEnvVar(name);
        if (!candidate.empty() && fs::exists(candidate))
            return candidate;
    }
    return L"";
}

static bool IsCodeFile(const fs::path& path)
{
    if (path.empty() || !path.has_extension())
        return false;
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
    for (const auto* codeExt : { L".cpp", L".c", L".cc", L".h", L".hpp", L".inl", L".cs", L".hlsl", L".glsl", L".shader" })
    {
        if (ext == codeExt)
            return true;
    }
    return false;
}

static void OpenInCodeEditor(HWND hwnd, const fs::path& path)
{
    if (path.empty() || !fs::exists(path) || fs::is_directory(path))
        return;
    std::wstring editor = GetEditorExecutable();
    if (!editor.empty())
    {
        std::wstring args = L"\"" + path.wstring() + L"\"";
        HINSTANCE result = ShellExecuteW(hwnd, L"open", editor.c_str(), args.c_str(), nullptr, SW_SHOWNORMAL);
        if ((INT_PTR)result <= 32)
            AddLog(L"[editor] failed to open " + path.filename().wstring());
        else
            AddLog(L"[editor] opened " + path.filename().wstring());
        return;
    }
    HINSTANCE result = ShellExecuteW(hwnd, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32)
        AddLog(L"[editor] failed to open " + path.filename().wstring());
    else
        AddLog(L"[editor] opened " + path.filename().wstring());
}

static void SetCursorVisibility(bool visible)
{
    if (visible)
    {
        while (ShowCursor(TRUE) < 0) {}
    }
    else
    {
        while (ShowCursor(FALSE) >= 0) {}
    }
}

static void StopMouseLook()
{
    if (!g_mouseLook) return;
    g_mouseLook = false;
    ReleaseCapture();
    SetCursorVisibility(true);
    ClipCursor(nullptr);
}

static void StartMouseLook(HWND hwnd, POINT pt)
{
    if (g_mouseLook) return;
    g_mouseLook = true;
    g_mouseLookLast = pt;
    SetCapture(hwnd);
    SetFocus(hwnd);
    SetCursorVisibility(false);
    RECT clip = g_sceneBodyRect;
    POINT tl{ clip.left, clip.top };
    POINT br{ clip.right, clip.bottom };
    ClientToScreen(hwnd, &tl);
    ClientToScreen(hwnd, &br);
    clip.left = tl.x;
    clip.top = tl.y;
    clip.right = br.x;
    clip.bottom = br.y;
    g_mouseLookCenter = { (clip.left + clip.right) / 2, (clip.top + clip.bottom) / 2 };
    ClipCursor(&clip);
    SetCursorPos(g_mouseLookCenter.x, g_mouseLookCenter.y);
}

static fs::path GetMainScenePath()
{
    if (g_projectRoot.empty())
        return fs::path();
    fs::path scene = g_projectRoot / L"scene.json";
    if (fs::exists(scene))
        return scene;
    fs::path fallback = g_projectRoot / L"Scenes" / L"main.scene.json";
    if (fs::exists(fallback))
        return fallback;
    return scene;
}

static std::string ToScenePath(const fs::path& path)
{
    try
    {
        fs::path rel = fs::relative(path, g_projectRoot);
        return rel.generic_string();
    }
    catch (...)
    {
        return path.generic_string();
    }
}

static bool AppendEntityToScene(const fs::path& assetPath)
{
    if (g_projectRoot.empty() || assetPath.empty())
        return false;
    fs::path scenePath = GetMainScenePath();
    if (scenePath.empty())
        return false;
    std::ifstream in(scenePath);
    if (!in.is_open())
        return false;
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    size_t entitiesPos = text.find("\"entities\"");
    if (entitiesPos == std::string::npos)
        return false;
    size_t openPos = text.find('[', entitiesPos);
    if (openPos == std::string::npos)
        return false;
    size_t closePos = text.rfind(']');
    if (closePos == std::string::npos || closePos < openPos)
        return false;

    std::wstring extW = assetPath.extension().wstring();
    std::transform(extW.begin(), extW.end(), extW.begin(), towlower);
    std::string ext(extW.begin(), extW.end());
    std::string relPath = ToScenePath(assetPath);
    std::ostringstream entry;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".dds")
    {
        entry << "    { \"id\": \"sprite_" << assetPath.stem().string()
              << "\", \"type\": \"Sprite\", \"sprite\": \"" << relPath
              << "\", \"position\": [0, 0, 0] }";
    }
    else if (ext == ".fbx" || ext == ".obj" || ext == ".blend" || ext == ".gltf" || ext == ".glb")
    {
        entry << "    { \"id\": \"mesh_" << assetPath.stem().string()
              << "\", \"type\": \"Mesh\", \"mesh\": \"" << relPath
              << "\", \"position\": [0, 0, 0] }";
    }
    else
    {
        return false;
    }

    bool needsComma = text.find('{', openPos) != std::string::npos && closePos > openPos + 1;
    std::string insert = (needsComma ? ",\n" : "\n");
    insert += entry.str();
    insert += "\n";

    text.insert(closePos, insert);
    std::ofstream out(scenePath, std::ios::trunc);
    if (!out.is_open())
        return false;
    out << text;
    out.close();
    return true;
}

static float ClampFloat(float value, float lo, float hi)
{
    return std::max(lo, std::min(value, hi));
}

static void MoveCamera(float forward, float right, float up)
{
    float cy = cosf(g_camYaw);
    float sy = sinf(g_camYaw);
    float cp = cosf(g_camPitch);
    float sp = sinf(g_camPitch);

    float fx = sy * cp;
    float fy = sp;
    float fz = cy * cp;
    float rx = cy;
    float rz = -sy;

    g_camX += fx * forward + rx * right;
    g_camY += fy * forward + up;
    g_camZ += fz * forward + rz * right;
}

struct HierItem
{
    std::wstring label;
    int depth{};
    bool isGroup{};
};

static void BuildHierarchyList(std::vector<HierItem>& out);

static bool IsHierarchyDeleted(const std::wstring& label)
{
    auto it = g_hierarchyDeleted.find(label);
    return it != g_hierarchyDeleted.end() && it->second;
}

static void DeleteHierarchySelection()
{
    std::vector<HierItem> list;
    BuildHierarchyList(list);
    if (list.empty()) return;
    bool deletedAny = false;
    for (const auto& item : list)
    {
        if (item.isGroup || item.label == L"Scene")
            continue;
        if (g_hierarchySelectedSet.find(item.label) == g_hierarchySelectedSet.end())
            continue;
        g_hierarchyDeleted[item.label] = true;
        g_hierarchyUndoStack.push_back(item.label);
        deletedAny = true;
    }
    if (!deletedAny && g_hierarchySelected >= 0 && g_hierarchySelected < (int)list.size())
    {
        const auto& item = list[g_hierarchySelected];
        if (!item.isGroup && item.label != L"Scene")
        {
            g_hierarchyDeleted[item.label] = true;
            g_hierarchyUndoStack.push_back(item.label);
        }
    }
}

static bool UndoHierarchyDelete()
{
    if (g_hierarchyUndoStack.empty())
        return false;
    std::wstring label = g_hierarchyUndoStack.back();
    g_hierarchyUndoStack.pop_back();
    g_hierarchyDeleted[label] = false;
    return true;
}

static std::wstring MakeUniqueGroupName()
{
    std::wstring base = L"Group";
    if (std::find(g_hierarchyGroups.begin(), g_hierarchyGroups.end(), base) == g_hierarchyGroups.end())
        return base;
    for (int i = 2; i < 1000; ++i)
    {
        std::wstring name = base + std::to_wstring(i);
        if (std::find(g_hierarchyGroups.begin(), g_hierarchyGroups.end(), name) == g_hierarchyGroups.end())
            return name;
    }
    return base;
}

static bool IsGroupExpanded(const std::wstring& label)
{
    auto it = g_hierarchyGroupExpanded.find(label);
    return it == g_hierarchyGroupExpanded.end() ? true : it->second;
}

static void BuildHierarchyList(std::vector<HierItem>& out)
{
    out.clear();
    out.push_back({ L"Scene", 0, true });
    if (g_hierarchySceneExpanded)
    {
        for (const auto& g : g_hierarchyGroups)
        {
            if (IsHierarchyDeleted(g)) continue;
            out.push_back({ g, 1, true });
            if (!IsGroupExpanded(g))
                continue;
            for (const auto& item : { L"Camera", L"Light", L"CenterBall", L"Orbiter" })
            {
                if (IsHierarchyDeleted(item)) continue;
                auto it = g_hierarchyParent.find(item);
                if (it != g_hierarchyParent.end() && it->second == g)
                    out.push_back({ item, 2, false });
            }
        }
        for (const auto& item : { L"Camera", L"Light", L"CenterBall", L"Orbiter" })
        {
            if (IsHierarchyDeleted(item)) continue;
            auto it = g_hierarchyParent.find(item);
            if (it == g_hierarchyParent.end() || it->second.empty() || it->second == L"Scene")
                out.push_back({ item, 1, false });
        }
    }
}

static std::wstring GetHierarchyLabel(const std::wstring& base)
{
    if (base == L"Scene") return base;
    auto it = g_hierarchyRenameMap.find(base);
    if (it != g_hierarchyRenameMap.end())
        return it->second;
    return base;
}

static std::wstring PathKey(const fs::path& p)
{
    return p.lexically_normal().wstring();
}

static void ClearFileSelection()
{
    g_fileSelected.clear();
}

static void SelectFileIndex(int idx, bool add, bool range)
{
    if (idx < 0 || idx >= (int)g_files.size()) return;
    if (range && g_fileAnchor >= 0 && g_fileAnchor < (int)g_files.size())
    {
        g_fileSelected.clear();
        int a = std::min(g_fileAnchor, idx);
        int b = std::max(g_fileAnchor, idx);
        for (int i = a; i <= b; ++i)
            g_fileSelected.insert(PathKey(g_files[i].path));
    }
    else
    {
        auto key = PathKey(g_files[idx].path);
        if (!add && g_fileSelected.find(key) == g_fileSelected.end())
            g_fileSelected.clear();
        if (add && g_fileSelected.find(key) != g_fileSelected.end())
            g_fileSelected.erase(key);
        else
            g_fileSelected.insert(key);
        g_fileAnchor = idx;
    }
    g_selectedIndex = idx;
}

static bool IsFileSelected(const FileEntry& e)
{
    return g_fileSelected.find(PathKey(e.path)) != g_fileSelected.end();
}

static std::vector<fs::path> GetSelectedFilePaths()
{
    std::vector<fs::path> out;
    for (const auto& e : g_files)
    {
        if (IsFileSelected(e))
            out.push_back(e.path);
    }
    return out;
}

static bool RectsIntersect(const RECT& a, const RECT& b)
{
    return !(a.right < b.left || a.left > b.right || a.bottom < b.top || a.top > b.bottom);
}

static void ApplyMarqueeSelection(const RECT& r, int target)
{
    if (target == 1)
    {
        g_fileSelected.clear();
        g_selectedIndex = -1;
        int y = g_projectListRect.top + 4;
        for (int i = 0; i < (int)g_files.size(); ++i)
        {
            RECT row{ g_projectListRect.left + 2, y - 2, g_projectListRect.right - 2, y + kLineH };
            if (RectsIntersect(r, row))
            {
                g_fileSelected.insert(PathKey(g_files[i].path));
                if (g_selectedIndex < 0)
                    g_selectedIndex = i;
            }
            y += kLineH;
            if (y > g_projectListRect.bottom) break;
        }
    }
    else if (target == 2)
    {
        g_hierarchySelectedSet.clear();
        g_hierarchySelected = -1;
        std::vector<HierItem> list;
        BuildHierarchyList(list);
        int y = g_leftListRect.top + 4;
        for (int i = 0; i < (int)list.size(); ++i)
        {
            RECT row{ g_leftListRect.left + 2, y - 2, g_leftListRect.right - 2, y + kLineH };
            if (RectsIntersect(r, row))
            {
                g_hierarchySelectedSet.insert(list[i].label);
                if (g_hierarchySelected < 0)
                    g_hierarchySelected = i;
            }
            y += kLineH;
            if (y > g_leftListRect.bottom) break;
        }
    }
}

static void SelectHierarchyIndex(int idx, bool add, bool range)
{
    std::vector<HierItem> list;
    BuildHierarchyList(list);
    if (idx < 0 || idx >= (int)list.size()) return;
    if (range && g_hierarchyAnchor >= 0 && g_hierarchyAnchor < (int)list.size())
    {
        g_hierarchySelectedSet.clear();
        int a = std::min(g_hierarchyAnchor, idx);
        int b = std::max(g_hierarchyAnchor, idx);
        for (int i = a; i <= b; ++i)
            g_hierarchySelectedSet.insert(list[i].label);
    }
    else
    {
        auto key = list[idx].label;
        if (!add && g_hierarchySelectedSet.find(key) == g_hierarchySelectedSet.end())
            g_hierarchySelectedSet.clear();
        if (add && g_hierarchySelectedSet.find(key) != g_hierarchySelectedSet.end())
            g_hierarchySelectedSet.erase(key);
        else
            g_hierarchySelectedSet.insert(key);
        g_hierarchyAnchor = idx;
    }
    g_hierarchySelected = idx;
}

void ParseCommandLine()
{
    std::wstring cmd = GetCommandLineW();
    if (cmd.find(L"--smoke") != std::wstring::npos || cmd.find(L"/smoke") != std::wstring::npos)
        g_smokeMode = true;

    const std::wstring token = L"--seconds=";
    size_t pos = cmd.find(token);
    if (pos != std::wstring::npos)
    {
        std::wstring num = cmd.substr(pos + token.size());
        size_t end = num.find_first_not_of(L"0123456789");
        if (end != std::wstring::npos)
            num = num.substr(0, end);
        int value = _wtoi(num.c_str());
        if (value > 0 && value < 600)
            g_smokeSeconds = value;
    }
}

void AddLog(const std::wstring& line)
{
    g_logs.push_back(line);
    if (g_logs.size() > kConsoleLinesMax)
        g_logs.erase(g_logs.begin());
    if (g_consoleWnd)
        InvalidateRect(g_consoleWnd, nullptr, FALSE);
    if (g_mainWnd)
        InvalidateRect(g_mainWnd, nullptr, FALSE);
}

void EnableDarkTitleBar(HWND hwnd)
{
    BOOL useDark = TRUE;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    if (FAILED(DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark))))
    {
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &useDark, sizeof(useDark));
    }
}

void BuildMenus()
{
    g_menuFile = CreatePopupMenu();
    AppendMenuW(g_menuFile, MF_STRING, 2001, L"New Folder");
    AppendMenuW(g_menuFile, MF_STRING, 2004, L"New File");
    AppendMenuW(g_menuFile, MF_STRING, 2002, L"Refresh");
    AppendMenuW(g_menuFile, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menuFile, MF_STRING, 2003, L"Exit");

    g_menuEdit = CreatePopupMenu();
    AppendMenuW(g_menuEdit, MF_STRING, 2101, L"Preferences...");

    g_menuView = CreatePopupMenu();
    AppendMenuW(g_menuView, MF_STRING, ID_TOGGLE_PROJECT, L"Project");
    AppendMenuW(g_menuView, MF_STRING, ID_TOGGLE_SCENE, L"Scene");
    AppendMenuW(g_menuView, MF_STRING, ID_TOGGLE_INSPECTOR, L"Inspector");
    AppendMenuW(g_menuView, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menuView, MF_STRING, ID_TOGGLE_CONSOLE, L"Console");
    AppendMenuW(g_menuView, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menuView, MF_STRING, ID_VIEW_CLOSE_ALL, L"Close All Panels");
    AppendMenuW(g_menuView, MF_STRING, ID_VIEW_RESET_LAYOUT, L"Reset Layout");

    g_menuWindow = CreatePopupMenu();
    AppendMenuW(g_menuWindow, MF_STRING, ID_TOGGLE_PROJECT, L"Project");
    AppendMenuW(g_menuWindow, MF_STRING, ID_TOGGLE_SCENE, L"Scene");
    AppendMenuW(g_menuWindow, MF_STRING, ID_TOGGLE_INSPECTOR, L"Inspector");
    AppendMenuW(g_menuWindow, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menuWindow, MF_STRING, ID_TOGGLE_CONSOLE, L"Console");
    AppendMenuW(g_menuWindow, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menuWindow, MF_STRING, ID_VIEW_CLOSE_ALL, L"Close All Panels");
    AppendMenuW(g_menuWindow, MF_STRING, ID_VIEW_RESET_LAYOUT, L"Reset Layout");

    g_menuHelp = CreatePopupMenu();
    AppendMenuW(g_menuHelp, MF_STRING, 2401, L"About");
}

void ShowMenuPopup(HWND hwnd, HMENU menu, int x, int y)
{
    SetForegroundWindow(hwnd);
    POINT pt{ x, y };
    ClientToScreen(hwnd, &pt);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessage(hwnd, WM_NULL, 0, 0);
}

static bool RectHasArea(const RECT& r)
{
    return (r.right > r.left) && (r.bottom > r.top);
}

static LRESULT CALLBACK RenameEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN)
    {
        if (wParam == VK_RETURN)
        {
            SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hwnd);
            return 0;
        }
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
            return 0;
        }
    }
    if (msg == WM_LBUTTONDOWN)
    {
        SendMessageW(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    if (msg == WM_SETFOCUS)
    {
        SendMessageW(hwnd, EM_SETSEL, 0, -1);
    }
    return CallWindowProc(g_oldEditProc, hwnd, msg, wParam, lParam);
}

void EndInlineRename()
{
    if (g_renameEdit)
    {
        DestroyWindow(g_renameEdit);
        g_renameEdit = nullptr;
        g_renameIndex = -1;
        g_oldEditProc = nullptr;
        g_renameTarget = 0;
        g_renameHierIndex = -1;
    }
}

void CommitInlineRename()
{
    if (!g_renameEdit) return;
    wchar_t buf[MAX_PATH]{};
    GetWindowTextW(g_renameEdit, buf, MAX_PATH);
    std::wstring name = buf;
    int target = g_renameTarget;
    int hierarchyIndex = g_hierarchySelected;
    EndInlineRename();
    if (!name.empty())
    {
        if (target == 0)
            RenameSelected(name);
        else if (target == 1)
        {
            std::vector<HierItem> list;
            BuildHierarchyList(list);
            if (hierarchyIndex >= 0 && hierarchyIndex < (int)list.size())
            {
                if (!list[hierarchyIndex].isGroup && list[hierarchyIndex].label != L"Scene")
                    g_hierarchyRenameMap[list[hierarchyIndex].label] = name;
            }
        }
    }
}

void BeginInlineRename(HWND hwnd)
{
    if (g_selectedIndex < 0 || g_selectedIndex >= (int)g_files.size())
        return;
    if (g_renameEdit) return;

    RECT rc = g_projectListRect;
    int y = rc.top + 4 + g_selectedIndex * kLineH;
    int x = rc.left + 10 + g_files[g_selectedIndex].depth * 12;
    if (g_files[g_selectedIndex].isDir)
        x += 12 + 38;
    else
        x += 40;
    int w = rc.right - x - 8;
    if (w < 80) w = 80;

    g_renameTarget = 0;
    g_renameIndex = g_selectedIndex;
    g_renameEdit = CreateWindowExW(
        0, L"EDIT", g_files[g_selectedIndex].path.filename().wstring().c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
        x - 2, y - 2, w + 4, kLineH + 4, hwnd, nullptr,
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);

    g_oldEditProc = (WNDPROC)SetWindowLongPtr(g_renameEdit, GWLP_WNDPROC, (LONG_PTR)RenameEditProc);
    SetFocus(g_renameEdit);
    SendMessageW(g_renameEdit, EM_SETSEL, 0, -1);
}

void BeginHierarchyRename(HWND hwnd)
{
    if (!g_showLeftPanel) return;
    if (g_hierarchySelected < 0) return;
    if (g_renameEdit) return;

    std::vector<HierItem> list;
    BuildHierarchyList(list);
    if (g_hierarchySelected >= (int)list.size()) return;
    if (list[g_hierarchySelected].isGroup) return;

    int y = g_leftListRect.top + 4 + g_hierarchySelected * kLineH;
    int x = g_leftListRect.left + 8 + list[g_hierarchySelected].depth * 12 + 12;
    int w = g_leftListRect.right - x - 8;
    if (w < 80) w = 80;

    std::wstring currentName = list[g_hierarchySelected].label;
    g_renameTarget = 1;
    g_renameHierIndex = g_hierarchySelected;
    g_renameEdit = CreateWindowExW(
        0, L"EDIT", currentName.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
        x - 2, y - 2, w + 4, kLineH + 4, hwnd, nullptr,
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);

    g_oldEditProc = (WNDPROC)SetWindowLongPtr(g_renameEdit, GWLP_WNDPROC, (LONG_PTR)RenameEditProc);
    SetFocus(g_renameEdit);
    SendMessageW(g_renameEdit, EM_SETSEL, 0, -1);
}
std::wstring PromptRename(HWND hwnd, const std::wstring& currentName)
{
    wchar_t buffer[MAX_PATH]{};
    wcsncpy_s(buffer, currentName.c_str(), _TRUNCATE);

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Rename";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    fs::path base = GetSelectedBase();
    std::wstring dir = base.wstring();
    ofn.lpstrInitialDir = dir.c_str();

    if (GetSaveFileNameW(&ofn))
    {
        fs::path p = buffer;
        return p.filename().wstring();
    }
    return L"";
}

void UpdateViewMenuChecks()
{
    if (!g_menuView) return;
    CheckMenuItem(g_menuView, ID_TOGGLE_PROJECT, MF_BYCOMMAND | (g_showProject ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_menuView, ID_TOGGLE_SCENE, MF_BYCOMMAND | (g_showScene ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_menuView, ID_TOGGLE_INSPECTOR, MF_BYCOMMAND | (g_showInspector ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_menuView, ID_TOGGLE_CONSOLE, MF_BYCOMMAND | (g_showConsole ? MF_CHECKED : MF_UNCHECKED));
    if (g_menuWindow)
    {
        CheckMenuItem(g_menuWindow, ID_TOGGLE_PROJECT, MF_BYCOMMAND | (g_showProject ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_menuWindow, ID_TOGGLE_SCENE, MF_BYCOMMAND | (g_showScene ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_menuWindow, ID_TOGGLE_INSPECTOR, MF_BYCOMMAND | (g_showInspector ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_menuWindow, ID_TOGGLE_CONSOLE, MF_BYCOMMAND | (g_showConsole ? MF_CHECKED : MF_UNCHECKED));
    }
}

LRESULT CALLBACK ConsoleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        g_consoleVisible = false;
        return 0;
    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(RGB(10, 10, 12));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(230, 230, 230));
        TextOutW(hdc, 10, 8, L"Console", 7);

        int y = 28;
        for (auto it = g_logs.rbegin(); it != g_logs.rend(); ++it)
        {
            if (y > rc.bottom - 14) break;
            TextOutW(hdc, 10, y, it->c_str(), (int)it->size());
            y += 14;
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(RGB(18, 18, 20));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(230, 230, 230));
        TextOutW(hdc, 16, 12, L"Settings", 8);
        SetTextColor(hdc, RGB(180, 180, 180));
        TextOutW(hdc, 16, 36, L"Editor Settings", 15);
        TextOutW(hdc, 16, 58, L"Project Settings", 16);
        TextOutW(hdc, 16, 80, L"Input / Shortcuts", 18);
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_mainWnd = hwnd;
        AddLog(L"[init] GameEditor started");
        char* userProfile = nullptr;
        size_t len = 0;
        _dupenv_s(&userProfile, &len, "USERPROFILE");
        fs::path project = userProfile ? fs::path(userProfile) / "GameEngineProjects" / "StarterProject"
                                       : fs::path("EngineFiles");
        if (userProfile)
            free(userProfile);
        if (fs::exists(project))
            SetProjectRoot(project);
        else
            SetProjectRoot(fs::path("EngineFiles"));
        RefreshFiles();
        if (g_smokeMode)
        {
            AddLog(L"[smoke] enabled");
        }
        g_codeEditorPath = GetEditorExecutable();

        const wchar_t CONSOLE_CLASS[] = L"GameEditorConsole";
        const wchar_t SETTINGS_CLASS[] = L"GameEditorSettings";
        WNDCLASSW cwc{};
        cwc.lpfnWndProc = ConsoleWndProc;
        cwc.hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        cwc.lpszClassName = CONSOLE_CLASS;
        cwc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&cwc);

        WNDCLASSW swc{};
        swc.lpfnWndProc = SettingsWndProc;
        swc.hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        swc.lpszClassName = SETTINGS_CLASS;
        swc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&swc);

        g_consoleWnd = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            CONSOLE_CLASS,
            L"GameEditor Console",
            WS_OVERLAPPEDWINDOW,
            50, 500, 900, 260,
            nullptr, nullptr, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);
        g_consoleVisible = false;

        g_settingsWnd = CreateWindowExW(
            WS_EX_TOPMOST,
            SETTINGS_CLASS,
            L"GameEditor Settings",
            WS_OVERLAPPEDWINDOW,
            120, 120, 520, 420,
            hwnd, nullptr, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);
        ShowWindow(g_settingsWnd, SW_HIDE);
        BuildMenus();
        UpdateViewMenuChecks();
        EnableDarkTitleBar(hwnd);
        EnableDarkTitleBar(g_consoleWnd);
        EnableDarkTitleBar(g_settingsWnd);

        DragAcceptFiles(hwnd, TRUE);

        g_startTick = GetTickCount64();
        g_lastTick = g_startTick;
        if (g_playing)
            SetTimer(hwnd, ID_ANIM_TIMER, 16, nullptr);
        SetTimer(hwnd, ID_NAV_TIMER, 16, nullptr);
        break;
    }
    case WM_ACTIVATEAPP:
        if (wParam)
        {
            RefreshFiles();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TOGGLE_CONSOLE)
        {
            g_showConsole = !g_showConsole;
            if (g_showConsole)
                g_consoleJustOpened = true;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_NEW_FILE)
        {
            CreateNewFile();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_NEW_FOLDER)
        {
            CreateNewFolder();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_COPY)
        {
            CopySelectionToClipboard();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_PASTE)
        {
            PasteClipboard();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_DELETE)
        {
            auto paths = GetSelectedFilePaths();
            if (paths.size() > 1)
                DeletePaths(paths);
            else
                DeleteSelected();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_RENAME)
        {
            BeginInlineRename(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_HIER_RENAME)
        {
            BeginHierarchyRename(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_HIER_DELETE)
        {
            DeleteHierarchySelection();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_CTX_HIER_NEW_GROUP)
        {
            std::wstring name = MakeUniqueGroupName();
            g_hierarchyGroups.push_back(name);
            g_hierarchyGroupExpanded[name] = true;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (HIWORD(wParam) == EN_KILLFOCUS && (HWND)lParam == g_renameEdit)
        {
            CommitInlineRename();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_TOGGLE_PROJECT)
        {
            g_showProject = !g_showProject;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_TOGGLE_SCENE)
        {
            g_showScene = !g_showScene;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_TOGGLE_INSPECTOR)
        {
            g_showInspector = !g_showInspector;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_VIEW_CLOSE_ALL)
        {
            g_showProject = false;
            g_showScene = false;
            g_showInspector = false;
            g_showConsole = false;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == ID_VIEW_RESET_LAYOUT)
        {
            g_showProject = true;
            g_showScene = true;
            g_showInspector = true;
            g_showConsole = true;
            g_projectCollapsed = true;
            g_bottomH = 220;
            g_consoleH = 110;
            g_consoleJustOpened = true;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == 2001)
        {
            CreateNewFolder();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == 2004)
        {
            CreateNewFile();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == 2002)
        {
            RefreshFiles();
            AddLog(L"[project] refreshed");
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == 2003)
        {
            PostQuitMessage(0);
        }
        else if (LOWORD(wParam) == 2101)
        {
            if (g_settingsWnd)
            {
                ShowWindow(g_settingsWnd, SW_SHOW);
                SetForegroundWindow(g_settingsWnd);
            }
        }
        else if (LOWORD(wParam) == 2401)
        {
            MessageBoxW(hwnd, L"GameEditor template\n(Win32 UI stub)", L"About", MB_OK | MB_ICONINFORMATION);
        }
        break;
    case WM_RBUTTONDOWN:
    {
        // Right-click in project panel: create folder under selected dir or root.
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (g_showLeftPanel && PtInRect(&g_leftListRect, pt))
        {
            std::vector<HierItem> list;
            BuildHierarchyList(list);
            int y = pt.y - g_leftListRect.top;
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)list.size())
            {
                SelectHierarchyIndex(idx, false, false);
                g_focusHierarchy = true;
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            HMENU ctx = CreatePopupMenu();
            AppendMenuW(ctx, MF_STRING, ID_CTX_HIER_RENAME, L"Rename");
            AppendMenuW(ctx, MF_STRING, ID_CTX_HIER_DELETE, L"Delete");
            AppendMenuW(ctx, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(ctx, MF_STRING, ID_CTX_HIER_NEW_GROUP, L"New Group");
            POINT screenPt{ pt.x, pt.y };
            ClientToScreen(hwnd, &screenPt);
            TrackPopupMenu(ctx, TPM_LEFTALIGN | TPM_TOPALIGN, screenPt.x, screenPt.y, 0, hwnd, nullptr);
            DestroyMenu(ctx);
            return 0;
        }
        if (PtInRect(&g_projectListRect, pt))
        {
            int y = pt.y - g_projectListRect.top;
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)g_files.size())
            {
                SelectFileIndex(idx, false, false);
                g_focusHierarchy = false;
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            HMENU ctx = CreatePopupMenu();
            AppendMenuW(ctx, MF_STRING, ID_CTX_NEW_FILE, L"New File");
            AppendMenuW(ctx, MF_STRING, ID_CTX_NEW_FOLDER, L"New Folder");
            AppendMenuW(ctx, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(ctx, MF_STRING, ID_CTX_COPY, L"Copy");
            AppendMenuW(ctx, MF_STRING, ID_CTX_PASTE, L"Paste");
            AppendMenuW(ctx, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(ctx, MF_STRING, ID_CTX_RENAME, L"Rename");
            AppendMenuW(ctx, MF_STRING, ID_CTX_DELETE, L"Delete");

            POINT screenPt{ pt.x, pt.y };
            ClientToScreen(hwnd, &screenPt);
            TrackPopupMenu(ctx, TPM_LEFTALIGN | TPM_TOPALIGN, screenPt.x, screenPt.y, 0, hwnd, nullptr);
            DestroyMenu(ctx);
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && g_mouseLook)
        {
            StopMouseLook();
            return 0;
        }
        if (wParam == VK_F1)
        {
            g_showConsole = !g_showConsole;
            if (g_showConsole)
                g_consoleJustOpened = true;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'C'))
        {
            CopySelectionToClipboard();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'V'))
        {
            PasteClipboard();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'Z'))
        {
            if (g_focusHierarchy && UndoHierarchyDelete())
                InvalidateRect(hwnd, nullptr, FALSE);
            else
                UndoLast();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'Y'))
        {
            RedoLast();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'S'))
        {
            SaveEditorState();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == '1'))
        {
            g_showLeftPanel = !g_showLeftPanel;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == '2'))
        {
            g_showRightPanel = !g_showRightPanel;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == '0'))
        {
            g_showLeftPanel = true;
            g_showRightPanel = true;
            g_leftPanelW = 260;
            g_rightPanelW = 260;
            g_sceneZoom = 1.0f;
            g_scenePanX = 0;
            g_scenePanY = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wParam == VK_F2)
        {
            if (g_showLeftPanel && g_hierarchySelected >= 0)
                BeginHierarchyRename(hwnd);
            else
                BeginInlineRename(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wParam == VK_DELETE)
        {
            if (g_focusHierarchy)
                DeleteHierarchySelection();
            else
            {
                auto paths = GetSelectedFilePaths();
                if (paths.size() > 1)
                    DeletePaths(paths);
                else
                    DeleteSelected();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wParam == 'I')
        {
            g_showInspector = !g_showInspector;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wParam == VK_SPACE)
        {
            if (!g_mouseLook)
            {
                g_projectCollapsed = !g_projectCollapsed;
                if (g_projectCollapsed)
                    g_prevBottomH = g_bottomH;
                else if (g_prevBottomH > 0)
                    g_bottomH = g_prevBottomH;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        else if (!g_playing && (wParam == 'W' || wParam == 'A' || wParam == 'S' || wParam == 'D' ||
                                wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT ||
                                wParam == VK_SPACE || wParam == VK_CONTROL || wParam == VK_SHIFT))
        {
            // Movement handled in the nav timer to avoid key repeat acceleration.
            return 0;
        }
        break;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE && g_mouseLook)
        {
            StopMouseLook();
            return 0;
        }
        break;
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(230, 230, 230));
        SetBkColor(hdc, RGB(24, 24, 28));
        if (!g_editBg)
            g_editBg = CreateSolidBrush(RGB(24, 24, 28));
        return (INT_PTR)g_editBg;
    }
    case WM_LBUTTONDOWN:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (g_renameEdit && !PtInRect(&g_projectListRect, pt) && !PtInRect(&g_leftListRect, pt))
        {
            CommitInlineRename();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        RECT rc; GetClientRect(hwnd, &rc);
        if (g_showLeftPanel)
        {
            int splitX = 8 + g_leftPanelW;
            RECT hit{ splitX - 6, kTopBarH, splitX + 6, rc.bottom };
            if (PtInRect(&hit, pt))
            {
                g_dragLeftSplit = true;
                SetCapture(hwnd);
                return 0;
            }
        }
        if (g_showRightPanel)
        {
            int splitX = rc.right - 8 - g_rightPanelW;
            RECT hit{ splitX - 6, kTopBarH, splitX + 6, rc.bottom };
            if (PtInRect(&hit, pt))
            {
                g_dragRightSplit = true;
                SetCapture(hwnd);
                return 0;
            }
        }
        if (PtInRect(&g_bottomSplitterRect, pt))
        {
            g_dragBottom = true;
            g_dragStartY = pt.y;
            g_dragStartH = g_bottomH;
            SetCapture(hwnd);
            return 0;
        }
        if (g_showConsole && PtInRect(&g_consoleSplitterRect, pt))
        {
            g_dragConsole = true;
            g_dragStartY = pt.y;
            g_dragStartH = g_consoleH;
            SetCapture(hwnd);
            return 0;
        }
        if (PtInRect(&g_btnPlay, pt))
        {
            g_playing = !g_playing;
            AddLog(g_playing ? L"[play] resumed" : L"[play] paused");
            if (g_playing && g_mouseLook)
            {
                StopMouseLook();
            }
            if (g_playing)
            {
                g_lastTick = GetTickCount64();
                SetTimer(hwnd, ID_ANIM_TIMER, 16, nullptr);
            }
            else
                KillTimer(hwnd, ID_ANIM_TIMER);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (PtInRect(&g_btnViewMode, pt))
        {
            g_view3D = !g_view3D;
            if (!g_view3D && g_mouseLook)
            {
                StopMouseLook();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (!g_playing && g_showScene && g_view3D && RectHasArea(g_sceneBodyRect) && PtInRect(&g_sceneBodyRect, pt))
        {
            StartMouseLook(hwnd, pt);
            return 0;
        }
        if (PtInRect(&g_btnLeftCollapse, pt))
        {
            g_showLeftPanel = !g_showLeftPanel;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (PtInRect(&g_btnRightCollapse, pt))
        {
            g_showRightPanel = !g_showRightPanel;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_showLeftPanel && PtInRect(&g_leftListRect, pt))
        {
            std::vector<HierItem> list;
            BuildHierarchyList(list);
            int y = pt.y - g_leftListRect.top;
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)list.size())
            {
                bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                SelectHierarchyIndex(idx, ctrl, shift);
                g_focusHierarchy = true;
                const auto& item = list[idx];
                int arrowX = g_leftListRect.left + 8 + item.depth * 12;
                if (item.isGroup && pt.x <= arrowX + 12)
                {
                    if (item.label == L"Scene")
                        g_hierarchySceneExpanded = !g_hierarchySceneExpanded;
                    else
                        g_hierarchyGroupExpanded[item.label] = !IsGroupExpanded(item.label);
                }
                if (!ctrl && !shift)
                {
                    g_dragHierarchy = true;
                    g_dragHierarchyActive = false;
                    g_hierarchyDragStart = pt;
                    g_hierarchyDragPos = pt;
                    g_hierarchyDragHoverIndex = -1;
                    SetCapture(hwnd);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else
            {
                g_focusHierarchy = true;
                g_hierarchySelectedSet.clear();
                g_hierarchySelected = -1;
                g_dragSelect = true;
                g_selectTarget = 2;
                g_selectStart = pt;
                g_selectRect = { pt.x, pt.y, pt.x, pt.y };
                SetCapture(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        // menu clicks
        for (int i = 0; i < (int)g_menuRects.size(); ++i)
        {
            if (PtInRect(&g_menuRects[i], pt))
            {
                int y = g_menuRects[i].bottom;
                int x = g_menuRects[i].left;
                switch (i)
                {
                case 0: ShowMenuPopup(hwnd, g_menuFile, x, y); break;
                case 1: ShowMenuPopup(hwnd, g_menuEdit, x, y); break;
                case 2: ShowMenuPopup(hwnd, g_menuView, x, y); break;
                case 3: ShowMenuPopup(hwnd, g_menuWindow, x, y); break;
                case 4: ShowMenuPopup(hwnd, g_menuHelp, x, y); break;
                }
                return 0;
            }
        }

        if (PtInRect(&g_projectListRect, pt))
        {
            int y = pt.y - g_projectListRect.top;
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)g_files.size())
            {
                bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                SelectFileIndex(idx, ctrl, shift);
                g_focusHierarchy = false;
                if (!ctrl && !shift)
                {
                    g_dragFile = true;
                    g_dragFileActive = false;
                    g_dragHoverIndex = -1;
                    g_dragStart = pt;
                    g_dragPos = pt;
                    SetCapture(hwnd);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else
            {
                g_focusHierarchy = false;
                g_fileSelected.clear();
                g_selectedIndex = -1;
                g_dragSelect = true;
                g_selectTarget = 1;
                g_selectStart = pt;
                g_selectRect = { pt.x, pt.y, pt.x, pt.y };
                SetCapture(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        if (PtInRect(&g_btnToggleConsole, pt))
        {
            g_showConsole = !g_showConsole;
            if (g_showConsole)
                g_consoleJustOpened = true;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (PtInRect(&g_btnToggleInspector, pt))
        {
            g_showInspector = !g_showInspector;
            UpdateViewMenuChecks();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (!g_projectCollapsed && PtInRect(&g_btnNewFile, pt))
        {
            CreateNewFile();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (!g_projectCollapsed && PtInRect(&g_btnNewFolder, pt))
        {
            CreateNewFolder();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (!g_projectCollapsed && PtInRect(&g_btnRefresh, pt))
        {
            RefreshFiles();
            AddLog(L"[project] refreshed");
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        return 0;
    }
    case WM_MBUTTONDOWN:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (PtInRect(&g_projectListRect, pt))
        {
            g_dragSelect = true;
            g_selectTarget = 1;
            g_selectStart = pt;
            g_selectRect = { pt.x, pt.y, pt.x, pt.y };
            SetCapture(hwnd);
            return 0;
        }
        if (g_showLeftPanel && PtInRect(&g_leftListRect, pt))
        {
            g_dragSelect = true;
            g_selectTarget = 2;
            g_selectStart = pt;
            g_selectRect = { pt.x, pt.y, pt.x, pt.y };
            SetCapture(hwnd);
            return 0;
        }
        g_panDrag = true;
        g_panLast = { pt.x, pt.y };
        SetCapture(hwnd);
        return 0;
    }
    case WM_MBUTTONUP:
        if (g_panDrag)
        {
            g_panDrag = false;
            ReleaseCapture();
            return 0;
        }
        break;
    case WM_LBUTTONDBLCLK:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (PtInRect(&g_projectListRect, pt))
        {
            int y = pt.y - g_projectListRect.top;
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)g_files.size())
            {
                const auto& e = g_files[idx];
                if (e.isDir)
                {
                    ToggleExpanded(e.path);
                    RefreshFiles();
                }
                else
                {
                    OpenInCodeEditor(hwnd, e.path);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }
    case WM_DROPFILES:
    {
        HDROP drop = (HDROP)wParam;
        UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        std::vector<fs::path> paths;
        paths.reserve(count);
        wchar_t buffer[MAX_PATH];
        for (UINT i = 0; i < count; ++i)
        {
            if (DragQueryFileW(drop, i, buffer, MAX_PATH))
            {
                paths.emplace_back(buffer);
            }
        }
        DragFinish(drop);
        ImportDropped(paths);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        if (g_playing)
            return 0;
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0)
            g_sceneZoom = std::min(2.5f, g_sceneZoom + 0.1f);
        else if (delta < 0)
            g_sceneZoom = std::max(0.3f, g_sceneZoom - 0.1f);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_TIMER:
        if (wParam == ID_ANIM_TIMER)
        {
            ULONGLONG now = GetTickCount64();
            double dt = (double)(now - g_lastTick) / 1000.0;
            g_lastTick = now;
            if (g_playing)
                InvalidateRect(hwnd, nullptr, FALSE);

            if (g_smokeMode)
            {
                ULONGLONG elapsed = now - g_startTick;
                if (elapsed > (ULONGLONG)g_smokeSeconds * 1000ULL)
                    PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
        }
        else if (wParam == ID_NAV_TIMER)
        {
            if (!g_playing)
            {
                if (g_view3D)
                {
                    float forward = 0.0f;
                    float right = 0.0f;
                    float speed = 0.08f;
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                        speed *= 3.0f;
                    float up = 0.0f;
                    if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000) forward += speed;
                    if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000) forward -= speed;
                    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) right += speed;
                    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) right -= speed;
                    if (GetAsyncKeyState(VK_SPACE) & 0x8000) up += speed;
                    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) up -= speed;
                    if (forward != 0.0f || right != 0.0f || up != 0.0f)
                    {
                        MoveCamera(forward, right, up);
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                }
                else
                {
                    int step = 4;
                    bool moved = false;
                    if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000) { g_scenePanY -= step; moved = true; }
                    if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000) { g_scenePanY += step; moved = true; }
                    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) { g_scenePanX -= step; moved = true; }
                    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) { g_scenePanX += step; moved = true; }
                    if (moved) InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
    {
        POINT pt{};
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        if (g_dragFileActive)
        {
            SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
            return TRUE;
        }
        if (g_showLeftPanel)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            int splitX = 8 + g_leftPanelW;
            RECT hit{ splitX - 6, kTopBarH, splitX + 6, rc.bottom };
            if (PtInRect(&hit, pt))
            {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }
        }
        if (g_showRightPanel)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            int splitX = rc.right - 8 - g_rightPanelW;
            RECT hit{ splitX - 6, kTopBarH, splitX + 6, rc.bottom };
            if (PtInRect(&hit, pt))
            {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }
        }
        if (g_showProject && RectHasArea(g_bottomSplitterRect) && PtInRect(&g_bottomSplitterRect, pt))
        {
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            return TRUE;
        }
        if (g_showProject && g_showConsole && RectHasArea(g_consoleSplitterRect) && PtInRect(&g_consoleSplitterRect, pt))
        {
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            return TRUE;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    case WM_MOUSEMOVE:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (g_dragSelect)
        {
            g_selectRect = { std::min(g_selectStart.x, pt.x), std::min(g_selectStart.y, pt.y),
                             std::max(g_selectStart.x, pt.x), std::max(g_selectStart.y, pt.y) };
            ApplyMarqueeSelection(g_selectRect, g_selectTarget);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragHierarchy)
        {
            int dx = abs(pt.x - g_hierarchyDragStart.x);
            int dy = abs(pt.y - g_hierarchyDragStart.y);
            if (dx > 4 || dy > 4)
                g_dragHierarchyActive = true;
            g_hierarchyDragPos = pt;
            if (PtInRect(&g_leftListRect, pt))
            {
                std::vector<HierItem> list;
                BuildHierarchyList(list);
                int y = pt.y - g_leftListRect.top;
                int idx = y / kLineH;
                if (idx >= 0 && idx < (int)list.size())
                    g_hierarchyDragHoverIndex = idx;
                else
                    g_hierarchyDragHoverIndex = -1;
            }
            else
            {
                g_hierarchyDragHoverIndex = -1;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (g_mouseLook && !g_playing)
        {
            POINT cur{};
            GetCursorPos(&cur);
            int dx = cur.x - g_mouseLookCenter.x;
            int dy = cur.y - g_mouseLookCenter.y;
            const float sensitivity = 0.0025f;
            if (dx != 0 || dy != 0)
            {
                g_camYaw += dx * sensitivity;
                g_camPitch = ClampFloat(g_camPitch - dy * sensitivity, -1.2f, 1.2f);
                SetCursorPos(g_mouseLookCenter.x, g_mouseLookCenter.y);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        if (g_panDrag && !g_playing)
        {
            int dx = pt.x - g_panLast.x;
            int dy = pt.y - g_panLast.y;
            g_panLast = pt;
            g_scenePanX += dx;
            g_scenePanY += dy;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragFile)
        {
            int dx = abs(pt.x - g_dragStart.x);
            int dy = abs(pt.y - g_dragStart.y);
            if (dx > 4 || dy > 4)
                g_dragFileActive = true;
            g_dragPos = pt;
            if (PtInRect(&g_projectListRect, pt))
            {
                int y = pt.y - g_projectListRect.top;
                int idx = y / kLineH;
                if (idx >= 0 && idx < (int)g_files.size())
                    g_dragHoverIndex = idx;
                else
                    g_dragHoverIndex = -1;
            }
            else
            {
                g_dragHoverIndex = -1;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (g_showLeftPanel && PtInRect(&g_leftListRect, pt))
        {
            int y = pt.y - g_leftListRect.top;
            std::vector<HierItem> list;
            BuildHierarchyList(list);
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)list.size())
            {
                if (g_hierarchyHover != idx)
                {
                    g_hierarchyHover = idx;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            else if (g_hierarchyHover != -1)
            {
                g_hierarchyHover = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        else if (g_hierarchyHover != -1)
        {
            g_hierarchyHover = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (g_dragLeftSplit)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            int newW = pt.x - 8;
            int maxW = (int)rc.right - 320;
            g_leftPanelW = std::max(160, std::min(newW, maxW));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragRightSplit)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            int newW = ((int)rc.right - 8) - pt.x;
            int maxW = (int)rc.right - 320;
            g_rightPanelW = std::max(160, std::min(newW, maxW));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragBottom)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            int dy = pt.y - g_dragStartY;
            int newH = g_dragStartH - dy;
            int maxH = rc.bottom - kTopBarH - kMinBottomH;
            g_bottomH = std::max(kMinBottomH, std::min(newH, maxH));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragConsole)
        {
            int dy = pt.y - g_dragStartY;
            int newH = g_dragStartH - dy;
            int maxConsole = std::max(0, g_bottomH - kPaneHeaderH - kSplitterH - 40);
            int minConsole = std::min(kMinConsoleH, maxConsole);
            g_consoleH = std::max(minConsole, std::min(newH, maxConsole));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        int hover = -1;
        for (int i = 0; i < (int)g_menuRects.size(); ++i)
        {
            if (PtInRect(&g_menuRects[i], pt))
            {
                hover = i;
                break;
            }
        }
        if (hover != g_hoverMenu)
        {
            g_hoverMenu = hover;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (PtInRect(&g_projectListRect, pt))
        {
            int y = pt.y - g_projectListRect.top;
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)g_files.size())
            {
                if (g_hoverFileIndex != idx)
                {
                    g_hoverFileIndex = idx;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            else if (g_hoverFileIndex != -1)
            {
                g_hoverFileIndex = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        else if (g_hoverFileIndex != -1)
        {
            g_hoverFileIndex = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }
    case WM_LBUTTONUP:
        if (g_dragSelect)
        {
            g_dragSelect = false;
            g_selectTarget = 0;
            ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragHierarchy)
        {
            ReleaseCapture();
            g_dragHierarchy = false;
            bool doMove = g_dragHierarchyActive;
            g_dragHierarchyActive = false;
            int dropIndex = g_hierarchyDragHoverIndex;
            g_hierarchyDragHoverIndex = -1;
            std::vector<HierItem> list;
            BuildHierarchyList(list);
            if (doMove && dropIndex >= 0 && dropIndex < (int)list.size())
            {
                const auto& target = list[dropIndex];
                if (target.isGroup && target.label != L"Scene")
                {
                    for (const auto& sel : g_hierarchySelectedSet)
                    {
                        g_hierarchyParent[sel] = target.label;
                    }
                }
                else
                {
                    for (const auto& sel : g_hierarchySelectedSet)
                    {
                        g_hierarchyParent.erase(sel);
                    }
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragFile)
        {
            ReleaseCapture();
            g_dragFile = false;
            bool doMove = g_dragFileActive;
            g_dragFileActive = false;
            int dropIndex = g_dragHoverIndex;
            g_dragHoverIndex = -1;
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            fs::path targetDir;
            if (doMove && g_showScene && RectHasArea(g_sceneBodyRect) && PtInRect(&g_sceneBodyRect, pt))
            {
                if (g_selectedIndex >= 0 && g_selectedIndex < (int)g_files.size())
                {
                    const auto& e = g_files[g_selectedIndex];
                    if (!e.isDir && AppendEntityToScene(e.path))
                    {
                        AddLog(L"[scene] added asset to scene");
                        RefreshFiles();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }
                doMove = false;
            }
            if (doMove && PtInRect(&g_projectListRect, pt))
            {
                int idx = dropIndex;
                if (idx < 0)
                {
                    int y = pt.y - g_projectListRect.top;
                    idx = y / kLineH;
                }
                if (idx >= 0 && idx < (int)g_files.size())
                {
                    const auto& e = g_files[idx];
                    targetDir = e.isDir ? e.path : e.path.parent_path();
                }
                else
                {
                    targetDir = GetProjectRoot();
                }
            }
            else if (doMove && PtInRect(&g_projectHeaderRect, pt))
            {
                targetDir = GetProjectRoot();
            }
            if (doMove && !targetDir.empty())
            {
                auto paths = GetSelectedFilePaths();
                if (paths.size() > 1)
                    MovePathsTo(paths, targetDir);
                else
                    MoveSelectedTo(targetDir);
            }
            if (!doMove && g_selectedIndex >= 0 && g_selectedIndex < (int)g_files.size())
            {
                const auto& e = g_files[g_selectedIndex];
                if (!e.isDir && IsCodeFile(e.path))
                    OpenInCodeEditor(hwnd, e.path);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_dragLeftSplit || g_dragRightSplit)
        {
            g_dragLeftSplit = false;
            g_dragRightSplit = false;
            ReleaseCapture();
            return 0;
        }
        if (g_dragBottom || g_dragConsole)
        {
            g_dragBottom = false;
            g_dragConsole = false;
            ReleaseCapture();
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        StopMouseLook();
        g_dragFile = false;
        g_dragFileActive = false;
        g_dragHoverIndex = -1;
        g_dragHierarchy = false;
        g_dragHierarchyActive = false;
        g_hierarchyDragHoverIndex = -1;
        g_dragSelect = false;
        g_selectTarget = 0;
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_KILLFOCUS:
        StopMouseLook();
        break;
    case WM_MOUSELEAVE:
        g_hoverMenu = -1;
        g_hoverFileIndex = -1;
        g_hierarchyHover = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_NCHITTEST:
    {
        // resize borders
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        RECT rc; GetClientRect(hwnd, &rc);
        bool left = pt.x < kResizeBorder;
        bool right = pt.x > rc.right - kResizeBorder;
        bool top = pt.y < kResizeBorder;
        bool bottom = pt.y > rc.bottom - kResizeBorder;
        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
        if (pt.y < kTopBarH)
        {
            if (PtInRect(&g_btnPlay, pt))
                return HTCLIENT;
            if (PtInRect(&g_btnViewMode, pt))
                return HTCLIENT;
            for (const auto& r : g_menuRects)
            {
                if (PtInRect(&r, pt))
                    return HTCLIENT;
            }
            return HTCAPTION;
        }
        if (RectHasArea(g_leftSplitRect) && PtInRect(&g_leftSplitRect, pt))
            return HTCLIENT;
        if (RectHasArea(g_rightSplitRect) && PtInRect(&g_rightSplitRect, pt))
            return HTCLIENT;
        return HTCLIENT;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(mem, bmp);

        RECT topbar{ 0,0,w,kTopBarH };
        int bottomH = g_showProject ? (g_projectCollapsed ? (kPaneHeaderH + 6) : g_bottomH) : 0;
        int workspaceBottom = h - bottomH;
        RECT workspace{ 0,kTopBarH,w,workspaceBottom };
        RECT bottom{ 0,workspaceBottom,w,h };
        g_bottomSplitterRect = g_showProject ? RECT{ 0, workspaceBottom - kSplitterH, w, workspaceBottom + kSplitterH } : RECT{};

        HBRUSH bg = CreateSolidBrush(RGB(8, 8, 10));
        FillRect(mem, &rc, bg);
        DeleteObject(bg);

        HBRUSH bar = CreateSolidBrush(RGB(18, 18, 20));
        FillRect(mem, &topbar, bar);
        DeleteObject(bar);

        if (g_showProject)
        {
            HBRUSH proj = CreateSolidBrush(RGB(14, 14, 16));
            FillRect(mem, &bottom, proj);
            DeleteObject(proj);

            g_projectHeaderRect = { 0, bottom.top, w, bottom.top + kPaneHeaderH };
            if (!g_projectCollapsed)
            {
            int consoleTotal = g_showConsole ? g_consoleH : 0;
            int maxConsole = std::max(0, bottomH - kPaneHeaderH - kSplitterH - 40);
            int minConsole = std::min(kMinConsoleH, maxConsole);
            if (g_showConsole)
            {
                if (g_consoleJustOpened)
                {
                    int target = (bottomH - kPaneHeaderH - kSplitterH) / 2;
                    g_consoleH = std::max(minConsole, std::min(target, maxConsole));
                    g_consoleJustOpened = false;
                }
                g_consoleH = std::max(minConsole, std::min(g_consoleH, maxConsole));
            }
            consoleTotal = g_showConsole ? g_consoleH : 0;

                int listBottom = bottom.bottom - (g_showConsole ? consoleTotal : 0);
                if (g_showConsole)
                    listBottom -= kSplitterH;
                g_projectListRect = { 0, g_projectHeaderRect.bottom, w, listBottom };

                if (g_showConsole)
                {
                    g_consoleSplitterRect = { 0, listBottom, w, listBottom + kSplitterH };
                    g_consoleHeaderRect = { 0, g_consoleSplitterRect.bottom, w, g_consoleSplitterRect.bottom + kPaneHeaderH };
                }
                else
                {
                    g_consoleSplitterRect = {};
                    g_consoleHeaderRect = {};
                }
            }
            else
            {
                g_projectListRect = {};
                g_consoleSplitterRect = {};
                g_consoleHeaderRect = {};
            }
        }
        else
        {
            g_projectHeaderRect = {};
            g_projectListRect = {};
            g_consoleSplitterRect = {};
            g_consoleHeaderRect = {};
        }

        SetBkMode(mem, TRANSPARENT);
        SetTextColor(mem, RGB(240, 240, 240));

        // Custom menu bar
        g_menuRects.clear();
        int x = 10;
        const wchar_t* labels[] = { L"File", L"Edit", L"View", L"Window", L"Help" };
        for (int i = 0; i < 5; ++i)
        {
            SIZE sz{};
            GetTextExtentPoint32W(mem, labels[i], (int)wcslen(labels[i]), &sz);
            RECT r{ x - 4, topbar.top + 4, x + sz.cx + 8, topbar.bottom - 4 };
            if (g_hoverMenu == i)
            {
                HBRUSH hover = CreateSolidBrush(RGB(45, 45, 48));
                FillRect(mem, &r, hover);
                DeleteObject(hover);
            }
            TextOutW(mem, x, topbar.top + 8, labels[i], (int)wcslen(labels[i]));
            g_menuRects.push_back(r);
            x += sz.cx + 20;
        }

        // Play + view mode buttons (OS close/minimize already in title bar)
        g_btnPlay = { w - 96, 4, w - 32, kTopBarH - 4 };
        g_btnViewMode = { g_btnPlay.left - 72, 4, g_btnPlay.left - 8, kTopBarH - 4 };
        HBRUSH btn = CreateSolidBrush(RGB(40, 40, 44));
        FillRect(mem, &g_btnPlay, btn);
        FillRect(mem, &g_btnViewMode, btn);
        DeleteObject(btn);
        SetTextColor(mem, RGB(230, 230, 230));
        const wchar_t* playBtn = g_playing ? L"Pause" : L"Play";
        TextOutW(mem, g_btnPlay.left + 10, g_btnPlay.top + 4, playBtn, (int)wcslen(playBtn));
        const wchar_t* viewBtn = g_view3D ? L"3D" : L"2D";
        TextOutW(mem, g_btnViewMode.left + 20, g_btnViewMode.top + 4, viewBtn, (int)wcslen(viewBtn));

        if (g_showProject)
        {
            std::wstring projectTitle = L"Project: " + GetProjectName();
            if (g_projectCollapsed)
                projectTitle += L" (collapsed) - Space to expand";
            DrawHeader(mem, g_projectHeaderRect, projectTitle.c_str());
            int btnH = 18;
            int btnY = g_projectHeaderRect.top + 2;
            int xRight = w - 8;
            g_btnToggleConsole = { xRight - 90, btnY, xRight, btnY + btnH };
            if (!g_projectCollapsed)
            {
                xRight = g_btnToggleConsole.left - 6;
                g_btnRefresh = { xRight - 70, btnY, xRight, btnY + btnH };
                xRight = g_btnRefresh.left - 6;
                g_btnNewFolder = { xRight - 90, btnY, xRight, btnY + btnH };
                xRight = g_btnNewFolder.left - 6;
                g_btnNewFile = { xRight - 70, btnY, xRight, btnY + btnH };
            }
            else
            {
                g_btnRefresh = {};
                g_btnNewFolder = {};
                g_btnNewFile = {};
            }

            HBRUSH footBtn = CreateSolidBrush(RGB(40, 40, 44));
            if (!g_projectCollapsed)
            {
                FillRect(mem, &g_btnNewFile, footBtn);
                FillRect(mem, &g_btnNewFolder, footBtn);
                FillRect(mem, &g_btnRefresh, footBtn);
            }
            FillRect(mem, &g_btnToggleConsole, footBtn);
            DeleteObject(footBtn);
            SetTextColor(mem, RGB(220, 220, 220));
            if (!g_projectCollapsed)
            {
                TextOutW(mem, g_btnNewFile.left + 6, g_btnNewFile.top + 3, L"New File", 8);
                TextOutW(mem, g_btnNewFolder.left + 6, g_btnNewFolder.top + 3, L"New Folder", 10);
                TextOutW(mem, g_btnRefresh.left + 8, g_btnRefresh.top + 3, L"Refresh", 7);
            }
            TextOutW(mem, g_btnToggleConsole.left + 6, g_btnToggleConsole.top + 3, g_showConsole ? L"Hide Console" : L"Show Console", g_showConsole ? 12 : 12);
        }

        if (g_showProject && !g_projectCollapsed)
        {
            int y = g_projectListRect.top + 4;
            for (int i = 0; i < (int)g_files.size(); ++i)
            {
                const auto& e = g_files[i];
                if (y > g_projectListRect.bottom - kLineH) break;
                if (i == g_hoverFileIndex)
                {
                    RECT hover{ g_projectListRect.left + 2, y - 2, g_projectListRect.right - 2, y + kLineH };
                    HBRUSH hBrush = CreateSolidBrush(RGB(32, 32, 36));
                    FillRect(mem, &hover, hBrush);
                    DeleteObject(hBrush);
                }
                if (IsFileSelected(e))
                {
                    RECT sel{ g_projectListRect.left + 2, y - 2, g_projectListRect.right - 2, y + kLineH };
                    HBRUSH selBrush = CreateSolidBrush(RGB(40, 40, 45));
                    FillRect(mem, &sel, selBrush);
                    DeleteObject(selBrush);
                }
                int x = g_projectListRect.left + 10 + e.depth * 12;
                if (e.isDir)
                {
                    TextOutW(mem, x, y, IsExpanded(e.path) ? L"v" : L">", 1);
                    x += 12;
                    TextOutW(mem, x, y, L"[DIR]", 5);
                    x += 38;
                }
                else
                {
                    std::wstring ext = e.path.extension().wstring();
                    for (auto& ch : ext) ch = (wchar_t)towlower(ch);
                    if (ext == L".c")
                        TextOutW(mem, x, y, L"[C]", 3);
                    else if (ext == L".cs")
                        TextOutW(mem, x, y, L"[CS]", 4);
                    else if (ext == L".cpp" || ext == L".cc" || ext == L".cxx")
                        TextOutW(mem, x, y, L"[C++]", 5);
                    else
                        TextOutW(mem, x, y, L"[FILE]", 6);
                    x += 40;
                }
                if (!(g_renameTarget == 0 && g_renameEdit && i == g_renameIndex))
                    TextOutW(mem, x, y, e.label.c_str(), (int)e.label.size());
                y += kLineH;
            }

            if (g_dragFileActive && g_dragHoverIndex >= 0 && g_dragHoverIndex < (int)g_files.size())
            {
                int hy = g_projectListRect.top + 4 + g_dragHoverIndex * kLineH;
                RECT drop{ g_projectListRect.left + 2, hy - 2, g_projectListRect.right - 2, hy + kLineH };
                HBRUSH dropBrush = CreateSolidBrush(RGB(70, 90, 140));
                FrameRect(mem, &drop, dropBrush);
                DeleteObject(dropBrush);
            }
            if (g_dragSelect && g_selectTarget == 1)
            {
                HBRUSH selBrush = CreateSolidBrush(RGB(120, 150, 200));
                FrameRect(mem, &g_selectRect, selBrush);
                DeleteObject(selBrush);
            }
        }

        if (RectHasArea(g_leftSplitRect))
        {
            HBRUSH split = CreateSolidBrush(RGB(28, 28, 32));
            FillRect(mem, &g_leftSplitRect, split);
            DeleteObject(split);
        }
        if (RectHasArea(g_rightSplitRect))
        {
            HBRUSH split = CreateSolidBrush(RGB(28, 28, 32));
            FillRect(mem, &g_rightSplitRect, split);
            DeleteObject(split);
        }

        if (g_dragFileActive && !g_fileSelected.empty())
        {
            std::wstring name = std::to_wstring(g_fileSelected.size()) + L" item(s)";
            int tx = g_dragPos.x + 16;
            int ty = g_dragPos.y + 16;
            RECT ghost{ tx, ty, tx + 240, ty + 22 };
            HBRUSH gbg = CreateSolidBrush(RGB(30, 30, 34));
            FillRect(mem, &ghost, gbg);
            DeleteObject(gbg);
            SetTextColor(mem, RGB(220, 220, 220));
            TextOutW(mem, tx + 6, ty + 4, name.c_str(), (int)name.size());
        }

        if (g_showProject && !g_projectCollapsed && bottomH > 0)
        {
            HBRUSH split = CreateSolidBrush(RGB(28, 28, 32));
            FillRect(mem, &g_bottomSplitterRect, split);

            if (g_showConsole)
            {
                FillRect(mem, &g_consoleSplitterRect, split);

                DrawHeader(mem, g_consoleHeaderRect, L"Console");
                RECT consoleBody{ 0, g_consoleHeaderRect.bottom, w, bottom.bottom };
                HBRUSH cBg = CreateSolidBrush(RGB(12, 12, 14));
                FillRect(mem, &consoleBody, cBg);
                DeleteObject(cBg);

                SetTextColor(mem, RGB(220, 220, 220));
                int cy = consoleBody.top + 6;
                for (auto it = g_logs.rbegin(); it != g_logs.rend(); ++it)
                {
                    if (cy > consoleBody.bottom - 14) break;
                    TextOutW(mem, 8, cy, it->c_str(), (int)it->size());
                    cy += 14;
                }
            }
            DeleteObject(split);
        }

        int leftW = g_showLeftPanel ? g_leftPanelW : 0;
        int rightW = g_showRightPanel ? g_rightPanelW : 0;
        RECT leftPanel{ workspace.left + 8, workspace.top + 8, workspace.left + 8 + leftW, workspace.bottom - 8 };
        RECT rightPanel{ workspace.right - 8 - rightW, workspace.top + 8, workspace.right - 8, workspace.bottom - 8 };
        RECT sceneHeader{ leftPanel.right + 8, workspace.top + 8, rightPanel.left - 8, workspace.top + 8 + kPaneHeaderH };
        RECT sceneBody{ leftPanel.right + 8, sceneHeader.bottom, rightPanel.left - 8, workspace.bottom - 8 };
        g_sceneBodyRect = sceneBody;
        g_leftSplitRect = g_showLeftPanel ? RECT{ leftPanel.right + 2, leftPanel.top, leftPanel.right + 6, leftPanel.bottom } : RECT{};
        g_rightSplitRect = g_showRightPanel ? RECT{ rightPanel.left - 6, rightPanel.top, rightPanel.left - 2, rightPanel.bottom } : RECT{};

        // Left dock: hierarchy tree
        if (g_showLeftPanel)
        {
            HBRUSH leftBg = CreateSolidBrush(RGB(16, 16, 20));
            FillRect(mem, &leftPanel, leftBg);
            DeleteObject(leftBg);

            RECT leftHeader{ leftPanel.left, leftPanel.top, leftPanel.right, leftPanel.top + kPaneHeaderH };
            DrawHeader(mem, leftHeader, L"Hierarchy");
            g_btnLeftCollapse = { leftHeader.right - 20, leftHeader.top + 2, leftHeader.right - 6, leftHeader.top + 18 };
            HBRUSH lc = CreateSolidBrush(RGB(40, 40, 44));
            FillRect(mem, &g_btnLeftCollapse, lc);
            DeleteObject(lc);
            SetTextColor(mem, RGB(220, 220, 220));
            TextOutW(mem, g_btnLeftCollapse.left + 4, g_btnLeftCollapse.top + 2, L"<", 1);

            g_leftPanelRect = leftPanel;
            g_leftListRect = { leftPanel.left + 6, leftHeader.bottom + 6, leftPanel.right - 6, leftPanel.bottom - 6 };
            HBRUSH listBg = CreateSolidBrush(RGB(18, 18, 22));
            FillRect(mem, &g_leftListRect, listBg);
            DeleteObject(listBg);

            std::vector<HierItem> list;
            BuildHierarchyList(list);
            int ry = g_leftListRect.top + 4;
            for (int i = 0; i < (int)list.size(); ++i)
            {
                if (ry > g_leftListRect.bottom - kLineH) break;
                if (i == g_hierarchyHover)
                {
                    RECT h{ g_leftListRect.left + 2, ry - 2, g_leftListRect.right - 2, ry + kLineH };
                    HBRUSH hb = CreateSolidBrush(RGB(32, 32, 36));
                    FillRect(mem, &h, hb);
                    DeleteObject(hb);
                }
                if (g_hierarchySelectedSet.find(list[i].label) != g_hierarchySelectedSet.end())
                {
                    RECT s{ g_leftListRect.left + 2, ry - 2, g_leftListRect.right - 2, ry + kLineH };
                    HBRUSH sb = CreateSolidBrush(RGB(40, 40, 45));
                    FillRect(mem, &s, sb);
                    DeleteObject(sb);
                }
                int x = g_leftListRect.left + 8 + list[i].depth * 12;
                if (list[i].isGroup)
                {
                    TextOutW(mem, x, ry, g_hierarchySceneExpanded ? L"v" : L">", 1);
                    x += 12;
                }
                std::wstring label = GetHierarchyLabel(list[i].label);
                SetTextColor(mem, RGB(210, 210, 210));
                if (!(g_renameTarget == 1 && g_renameEdit && i == g_renameHierIndex))
                    TextOutW(mem, x, ry, label.c_str(), (int)label.size());
                ry += kLineH;
            }
            if (g_dragHierarchyActive && g_hierarchyDragHoverIndex >= 0 && g_hierarchyDragHoverIndex < (int)list.size())
            {
                int hy = g_leftListRect.top + 4 + g_hierarchyDragHoverIndex * kLineH;
                RECT drop{ g_leftListRect.left + 2, hy - 2, g_leftListRect.right - 2, hy + kLineH };
                HBRUSH dropBrush = CreateSolidBrush(RGB(70, 90, 140));
                FrameRect(mem, &drop, dropBrush);
                DeleteObject(dropBrush);
            }
            if (g_dragHierarchyActive)
            {
                std::wstring name = std::to_wstring(g_hierarchySelectedSet.size()) + L" item(s)";
                int tx = g_hierarchyDragPos.x + 16;
                int ty = g_hierarchyDragPos.y + 16;
                RECT ghost{ tx, ty, tx + 200, ty + 22 };
                HBRUSH gbg = CreateSolidBrush(RGB(30, 30, 34));
                FillRect(mem, &ghost, gbg);
                DeleteObject(gbg);
                SetTextColor(mem, RGB(220, 220, 220));
                TextOutW(mem, tx + 6, ty + 4, name.c_str(), (int)name.size());
            }
            if (g_dragSelect && g_selectTarget == 2)
            {
                HBRUSH selBrush = CreateSolidBrush(RGB(120, 150, 200));
                FrameRect(mem, &g_selectRect, selBrush);
                DeleteObject(selBrush);
            }
        }
        else
        {
            RECT tab{ workspace.left + 6, workspace.top + 8, workspace.left + 24, workspace.top + 8 + kPaneHeaderH };
            g_btnLeftCollapse = tab;
            HBRUSH tbg = CreateSolidBrush(RGB(22, 22, 26));
            FillRect(mem, &tab, tbg);
            DeleteObject(tbg);
            SetTextColor(mem, RGB(220, 220, 220));
            TextOutW(mem, tab.left + 5, tab.top + 2, L">", 1);
            g_leftPanelRect = {};
            g_leftListRect = {};
        }

        // Scene view (center)
        if (g_showScene)
        {
            DrawHeader(mem, sceneHeader, L"");
            bool showCenter = !IsHierarchyDeleted(L"CenterBall");
            bool showOrbit = !IsHierarchyDeleted(L"Orbiter");
            DrawScenePreview(mem, sceneBody, g_playing, g_sceneZoom, g_scenePanX, g_scenePanY, g_view3D, g_camYaw, g_camPitch, g_camX, g_camY, g_camZ, showCenter, showOrbit);
            SetTextColor(mem, RGB(160, 160, 160));
            const wchar_t* modeText = g_view3D ? L"3D mode (click view, ESC to release)" : L"2D mode";
            TextOutW(mem, sceneBody.left + 12, sceneBody.top + 12, modeText, (int)wcslen(modeText));
        }
        else
        {
            SetTextColor(mem, RGB(160, 160, 160));
            TextOutW(mem, sceneBody.left + 12, sceneBody.top + 12, L"Scene hidden (View menu)", 26);
        }

        // Right dock: variables panel
        if (g_showRightPanel)
        {
            HBRUSH rightBg = CreateSolidBrush(RGB(16, 16, 20));
            FillRect(mem, &rightPanel, rightBg);
            DeleteObject(rightBg);

            RECT rightHeader{ rightPanel.left, rightPanel.top, rightPanel.right, rightPanel.top + kPaneHeaderH };
            DrawHeader(mem, rightHeader, L"Variables");
            g_btnRightCollapse = { rightHeader.left + 6, rightHeader.top + 2, rightHeader.left + 20, rightHeader.top + 18 };
            HBRUSH rc = CreateSolidBrush(RGB(40, 40, 44));
            FillRect(mem, &g_btnRightCollapse, rc);
            DeleteObject(rc);
            SetTextColor(mem, RGB(220, 220, 220));
            TextOutW(mem, g_btnRightCollapse.left + 4, g_btnRightCollapse.top + 2, L">", 1);

            RECT tabBar{ rightPanel.left + 6, rightHeader.bottom + 6, rightPanel.right - 6, rightHeader.bottom + 28 };
            HBRUSH tabBg = CreateSolidBrush(RGB(24, 24, 28));
            FillRect(mem, &tabBar, tabBg);
            DeleteObject(tabBg);
            SetTextColor(mem, RGB(220, 220, 220));
            TextOutW(mem, tabBar.left + 6, tabBar.top + 4, L"Inspector", 9);
            TextOutW(mem, tabBar.left + 90, tabBar.top + 4, L"Scripts", 7);
            TextOutW(mem, tabBar.left + 160, tabBar.top + 4, L"Audio", 5);

            RECT rightContent{ rightPanel.left + 6, tabBar.bottom + 6, rightPanel.right - 6, rightPanel.bottom - 6 };
            HBRUSH rightContentBg = CreateSolidBrush(RGB(18, 18, 22));
            FillRect(mem, &rightContent, rightContentBg);
            DeleteObject(rightContentBg);
            SetTextColor(mem, RGB(200, 200, 200));
            int ly = rightContent.top + 6;
            TextOutW(mem, rightContent.left + 6, ly, L"Transform", 9); ly += 16;
            TextOutW(mem, rightContent.left + 16, ly, L"Position: (0,0,0)", 19); ly += 16;
            TextOutW(mem, rightContent.left + 16, ly, L"Rotation: (0,0,0)", 19); ly += 16;
            TextOutW(mem, rightContent.left + 16, ly, L"Scale: (1,1,1)", 16); ly += 20;
            TextOutW(mem, rightContent.left + 6, ly, L"Script: OrbitLogic.c", 22);
        }
        else
        {
            RECT tab{ workspace.right - 24, workspace.top + 8, workspace.right - 6, workspace.top + 8 + kPaneHeaderH };
            g_btnRightCollapse = tab;
            HBRUSH tbg = CreateSolidBrush(RGB(22, 22, 26));
            FillRect(mem, &tab, tbg);
            DeleteObject(tbg);
            SetTextColor(mem, RGB(220, 220, 220));
            TextOutW(mem, tab.left + 5, tab.top + 2, L"<", 1);
        }

        // Draw left collapse tab last so it stays on top of scene header
        if (!g_showLeftPanel && RectHasArea(g_btnLeftCollapse))
        {
            HBRUSH tbg = CreateSolidBrush(RGB(22, 22, 26));
            FillRect(mem, &g_btnLeftCollapse, tbg);
            DeleteObject(tbg);
            SetTextColor(mem, RGB(220, 220, 220));
            TextOutW(mem, g_btnLeftCollapse.left + 5, g_btnLeftCollapse.top + 2, L">", 1);
        }

        g_btnToggleInspector = {};

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_DESTROY:
        if (g_editBg)
        {
            DeleteObject(g_editBg);
            g_editBg = nullptr;
        }
        ClipCursor(nullptr);
        KillTimer(hwnd, ID_ANIM_TIMER);
        KillTimer(hwnd, ID_NAV_TIMER);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    ParseCommandLine();
    const wchar_t CLASS_NAME[] = L"GameEditorWindow";
        WNDCLASSW wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.style = CS_DBLCLKS;

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"GameEditor (Template)", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    EnableDarkTitleBar(hwnd);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
