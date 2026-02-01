#include <windows.h>
#include "../../GameEngineCore/App/PluginAPI.h"

static const EngineAPI* g_api = 0;
static double g_accum = 0.0;

__declspec(dllexport) bool GE_PluginInit(const EngineAPI* api)
{
    g_api = api;
    if (g_api && g_api->Log) g_api->Log("[ExampleCPlugin] init");
    return true;
}

__declspec(dllexport) void GE_PluginUpdate(double dt)
{
    g_accum += dt;
    if (g_accum >= 2.0)
    {
        g_accum = 0.0;
        if (g_api && g_api->Log) g_api->Log("[ExampleCPlugin] tick");
    }
}

__declspec(dllexport) void GE_PluginShutdown()
{
    if (g_api && g_api->Log) g_api->Log("[ExampleCPlugin] shutdown");
}
