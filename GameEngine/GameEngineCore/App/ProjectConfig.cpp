#include "ProjectConfig.h"

#include "AppState.h"

#include <windows.h>

static std::wstring g_projectPath;

static std::wstring GetArgValue(const std::wstring& cmd, const std::wstring& key)
{
    size_t pos = cmd.find(key);
    if (pos == std::wstring::npos) return L"";

    pos += key.size();
    if (pos >= cmd.size()) return L"";

    if (cmd[pos] == L'\"')
    {
        size_t end = cmd.find(L"\"", pos + 1);
        if (end == std::wstring::npos) return cmd.substr(pos + 1);
        return cmd.substr(pos + 1, end - (pos + 1));
    }

    size_t end = cmd.find_first_of(L" \t\r\n", pos);
    if (end == std::wstring::npos) return cmd.substr(pos);
    return cmd.substr(pos, end - pos);
}

bool InitProjectFromCommandLine()
{
    std::wstring cmd = GetCommandLineW();
    std::wstring value = GetArgValue(cmd, L"--project=");
    if (value.empty())
        value = GetArgValue(cmd, L"--project ");

    if (!value.empty())
    {
        g_projectPath = value;
        g_app.projectPath = value;
        return true;
    }

    g_projectPath.clear();
    g_app.projectPath.clear();
    return false;
}

const std::wstring& GetProjectPath()
{
    return g_projectPath;
}
