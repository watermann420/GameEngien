#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "msimg32.lib")

#include <sstream>

#include "App/AppState.h"
#include "App/AudioPlayback.h"
#include "App/VideoPlayback.h"
#include "App/OverlayWindow.h"
#include "App/ProjectConfig.h"
#include "App/PluginManager.h"
#include "Scripts/BlueBox2DTest.h"

static void InitApp()
{
    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    SetProcessDPIAware();

    g_app.renderer.RenderToBitmapFile(L"headless_output.bmp", AppState::kWindowWidth, AppState::kWindowHeight);
    InitProjectFromCommandLine();
    InitVideoFromFiles();

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    g_app.overlay = CreateOverlayWindow(hInstance, screenW, screenH);
    g_app.bounds = RECT{ 0,0,screenW,screenH };

    HDC screen = GetDC(nullptr);
    InitOverlayBuffers(screen);
    ReleaseDC(nullptr, screen);

    g_app.collider.Set((screenW - g_app.renderer.BoxWidth()) / 2,
                       (screenH - g_app.renderer.BoxHeight()) / 2,
                       g_app.renderer.BoxWidth(),
                       g_app.renderer.BoxHeight());

    InitBlueBoxText(g_app.bounds);
    ShowWindow(g_app.overlay, SW_SHOW);
}

static bool ProcessMessages(bool& running)
{
    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            running = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

static void ClearFrame()
{
    HBRUSH clearBrush = CreateSolidBrush(RGB(255, 0, 255));
    RECT clearRect{ 0,0,g_app.bounds.right,g_app.bounds.bottom };
    FillRect(g_app.memDC, &clearRect, clearBrush);
    DeleteObject(clearBrush);
}

static void RunLoop()
{
    HDC screen = GetDC(nullptr);
    ULONGLONG prevTicks = GetTickCount64();
    bool running = true;
    PluginManager plugins{};
    EngineAPI api{};
    api.Log = [](const char* message)
    {
        if (!message) return;
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
    };
    plugins.LoadAll(api);

    while (running)
    {
        if (!ProcessMessages(running)) break;

        ULONGLONG now = GetTickCount64();
        double dt = static_cast<double>(now - prevTicks) / 1000.0;
        prevTicks = now;

        ClearFrame();
        if (!UpdateVideoAndRender(dt, g_app.memDC))
            RenderFallback(g_app.memDC);

        plugins.UpdateAll(dt);
        UpdateOverlay(screen);
        Sleep(1);
    }

    plugins.ShutdownAll();
    ReleaseDC(nullptr, screen);
}

static void CleanupApp()
{
    CleanupOverlayBuffers();
    DestroyWindow(g_app.overlay);
    timeEndPeriod(1);
    CoUninitialize();
}

int main()
{
    InitApp();
    RunLoop();
    CleanupApp();
    return 0;
}
