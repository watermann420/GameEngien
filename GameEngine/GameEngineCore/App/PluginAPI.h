#pragma once

#include <cstdint>

struct EngineAPI
{
    void (*Log)(const char* message);
};

extern "C"
{
    __declspec(dllexport) bool GE_PluginInit(const EngineAPI* api);
    __declspec(dllexport) void GE_PluginUpdate(double dt);
    __declspec(dllexport) void GE_PluginShutdown();
}
