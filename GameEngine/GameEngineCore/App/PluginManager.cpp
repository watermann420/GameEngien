#include "PluginManager.h"

#include <windows.h>
#include <filesystem>

namespace fs = std::filesystem;

static void* LoadModule(const std::wstring& path)
{
    return (void*)LoadLibraryW(path.c_str());
}

static void UnloadModule(void* module)
{
    if (module) FreeLibrary((HMODULE)module);
}

static void* FindSymbol(void* module, const char* name)
{
    if (!module) return nullptr;
    return (void*)GetProcAddress((HMODULE)module, name);
}

bool PluginManager::LoadAll(const EngineAPI& api)
{
    const fs::path root = fs::path("EnginePlugins");
    if (!fs::exists(root)) return true;

    for (const auto& entry : fs::directory_iterator(root))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != L".dll") continue;

        LoadedPlugin plugin{};
        plugin.path = entry.path().wstring();
        plugin.module = LoadModule(plugin.path);
        if (!plugin.module) continue;

        plugin.init = (bool (*)(const EngineAPI*))FindSymbol(plugin.module, "GE_PluginInit");
        plugin.update = (void (*)(double))FindSymbol(plugin.module, "GE_PluginUpdate");
        plugin.shutdown = (void (*)())FindSymbol(plugin.module, "GE_PluginShutdown");

        if (!plugin.init || !plugin.update || !plugin.shutdown)
        {
            UnloadModule(plugin.module);
            continue;
        }

        if (!plugin.init(&api))
        {
            UnloadModule(plugin.module);
            continue;
        }

        plugins_.push_back(plugin);
    }

    return true;
}

void PluginManager::UpdateAll(double dt)
{
    for (auto& p : plugins_)
    {
        if (p.update) p.update(dt);
    }
}

void PluginManager::ShutdownAll()
{
    for (auto& p : plugins_)
    {
        if (p.shutdown) p.shutdown();
        UnloadModule(p.module);
    }
    plugins_.clear();
}
