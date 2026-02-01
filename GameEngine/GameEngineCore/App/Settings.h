#pragma once

#include <string>

struct KeyBindings
{
    std::wstring play = L"Space";
    std::wstring toggleConsole = L"F1";
    std::wstring rename = L"F2";
    std::wstring undo = L"Ctrl+Z";
    std::wstring redo = L"Ctrl+Y";
    std::wstring save = L"Ctrl+S";
};

struct EditorSettings
{
    KeyBindings keys;
};

void LoadEditorSettings();
const EditorSettings& GetEditorSettings();
