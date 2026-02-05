#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct FileEntry
{
    std::wstring label;
    fs::path path;
    bool isDir{ false };
    int depth{ 0 };
};

extern std::vector<FileEntry> g_files;
extern int g_selectedIndex;
extern fs::path g_projectRoot;

void AddLog(const std::wstring& line);
void SetProjectRoot(const fs::path& root);
const fs::path& GetProjectRoot();
std::wstring GetProjectName();
void CollectTree(const fs::path& root, int depth, std::vector<FileEntry>& out);
void RefreshFiles();
fs::path GetSelectedBase();
void CreateNewFolder();
void CreateNewFile();
void CopySelectionToClipboard();
void PasteClipboard();
void DeleteSelected();
void DeletePaths(const std::vector<fs::path>& paths);
void MovePathsTo(const std::vector<fs::path>& paths, const fs::path& targetDir);
void RenameSelected(const std::wstring& newName);
void ImportDropped(const std::vector<fs::path>& paths);
void MoveSelectedTo(const fs::path& targetDir);
void UndoLast();
void RedoLast();
bool CanUndo();
bool CanRedo();
void SaveEditorState();
bool ToggleExpanded(const fs::path& path);
bool IsExpanded(const fs::path& path);
void EnsureProjectLayout();
