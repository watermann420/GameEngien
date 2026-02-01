#pragma once

#include <string>
#include <vector>

#include "PluginAPI.h"

struct LoadedPlugin
{
    std::wstring path;
    void* module = nullptr;
    bool (*init)(const EngineAPI* api) = nullptr;
    void (*update)(double dt) = nullptr;
    void (*shutdown)() = nullptr;
};

class PluginManager
{
public:
    bool LoadAll(const EngineAPI& api);
    void UpdateAll(double dt);
    void ShutdownAll();

private:
    std::vector<LoadedPlugin> plugins_;
};
