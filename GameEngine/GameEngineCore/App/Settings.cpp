#include "Settings.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static EditorSettings g_settings{};

static fs::path GetSettingsPath()
{
    return fs::path("EngineFiles") / "settings.json";
}

void LoadEditorSettings()
{
    fs::path path = GetSettingsPath();
    if (!fs::exists(path)) return;

    std::ifstream in(path);
    if (!in) return;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.find("\"play\"") != std::string::npos) g_settings.keys.play = L"Space";
        if (line.find("\"toggleConsole\"") != std::string::npos) g_settings.keys.toggleConsole = L"F1";
        if (line.find("\"rename\"") != std::string::npos) g_settings.keys.rename = L"F2";
        if (line.find("\"undo\"") != std::string::npos) g_settings.keys.undo = L"Ctrl+Z";
        if (line.find("\"redo\"") != std::string::npos) g_settings.keys.redo = L"Ctrl+Y";
        if (line.find("\"save\"") != std::string::npos) g_settings.keys.save = L"Ctrl+S";
    }
}

const EditorSettings& GetEditorSettings()
{
    return g_settings;
}
