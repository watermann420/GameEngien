#include <windows.h>
#include <mmsystem.h>
#include <chrono>
#include <string>
#include <vector>
#include <cstdio>
#include <crtdbg.h>

#include "../../GameEngineCore/RenderPipeline2D/Renderer2D.h"
#include "../../GameEngineCore/Colliders/BoxCollider2D.h"

#pragma comment(lib, "winmm.lib")

static const int kWindowWidth = 800;
static const int kWindowHeight = 600;
static Renderer2D g_renderer(200, 200, RGB(0, 122, 255));
static BoxCollider2D g_collider;
static HWND g_overlay = nullptr;

struct Timer
{
    Timer() { Reset(); }
    void Reset() { start = std::chrono::high_resolution_clock::now(); }
    double Ms() const
    {
        using namespace std::chrono;
        return duration_cast<duration<double, std::milli>>(high_resolution_clock::now() - start).count();
    }
    std::chrono::high_resolution_clock::time_point start;
};

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    case WM_LBUTTONDOWN:
    {
        POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (g_collider.Contains(p))
        {
            PostQuitMessage(0); // close on click inside the box
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

HWND CreateOverlayWindow(HINSTANCE hInstance, int width, int height)
{
    const wchar_t* CLASS_NAME = L"GameEngineOverlayClass.Test";

    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        CLASS_NAME,
        L"GameEngine Smoke Test",
        WS_POPUP,
        0, 0, width, height,
        nullptr, nullptr, hInstance, nullptr);

    return hwnd;
}

void ShowBoxOverlay(HWND hwnd, int width, int height, COLORREF color)
{
    HDC screen = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, width, height);
    HGDIOBJ old = SelectObject(memDC, bmp);

    RECT area{ 0,0,width,height };
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(memDC, &area, brush);
    DeleteObject(brush);

    POINT pos{
        (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - height) / 2
    };
    SIZE size{ width, height };
    POINT src{ 0,0 };
    BLENDFUNCTION bf{};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = 0;

    UpdateLayeredWindow(hwnd, screen, &pos, &size, memDC, &src, 0, &bf, ULW_ALPHA);

    g_collider.Set(0, 0, width, height);

    SelectObject(memDC, old);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screen);

    ShowWindow(hwnd, SW_SHOW);
}

void RunHeadlessRender()
{
    Timer t;
    bool ok = g_renderer.RenderToBitmapFile(L"headless_output.bmp", kWindowWidth, kWindowHeight);
    std::printf("[headless] render %s in %.2f ms -> headless_output.bmp\n", ok ? "OK" : "FAILED", t.Ms());
}

void RunOverlaySmoke()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    g_overlay = CreateOverlayWindow(hInstance, g_renderer.BoxWidth(), g_renderer.BoxHeight());
    ShowBoxOverlay(g_overlay, g_renderer.BoxWidth(), g_renderer.BoxHeight(), g_renderer.Color());

    MSG msg{};
    Timer loopTimer;
    int frames = 0;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        ++frames;
        if (frames % 120 == 0)
        {
            double elapsed = loopTimer.Ms();
            std::printf("[overlay] %d messages in %.1f ms (%.1f msgs/s)\n", frames, elapsed, frames / (elapsed / 1000.0));
            loopTimer.Reset();
        }
    }

    DestroyWindow(g_overlay);
}

void RunAudioPing()
{
    // Try system alias; if it fails, fall back to Beep so the harness remains self-contained.
    if (PlaySoundW(L"SystemAsterisk", nullptr, SND_ALIAS | SND_SYNC))
    {
        std::printf("[audio] Played alias SystemAsterisk via winmm\n");
    }
    else
    {
        Beep(880, 150);
        std::printf("[audio] Fallback beep (winmm alias unavailable)\n");
    }
}

void RunPerfSample()
{
    const int iterations = 200;
    Timer timer;

    // Create an in-memory DC once for better signal.
    HDC screen = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screen);
    RECT area{ 0,0,kWindowWidth,kWindowHeight };
    HBITMAP bmp = CreateCompatibleBitmap(screen, kWindowWidth, kWindowHeight);
    HGDIOBJ old = SelectObject(memDC, bmp);

    for (int i = 0; i < iterations; ++i)
    {
        g_renderer.RenderToDC(memDC, area);
    }

    double totalMs = timer.Ms();
    std::printf("[perf] %d blits in %.2f ms (%.2f blits/s)\n", iterations, totalMs, iterations / (totalMs / 1000.0));

    SelectObject(memDC, old);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screen);
}

int main()
{
#if defined(_DEBUG)
    // Debug CRT leak detection.
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    std::printf("[init] CRT leak checks enabled (Debug)\n");
#else
    std::printf("[init] Release build (CRT leak report disabled)\n");
#endif

    SetProcessDPIAware();

    RunHeadlessRender(); // produces headless_output.bmp
    RunAudioPing();      // simple audibility check
    RunPerfSample();     // measure basic render throughput

    std::puts("[overlay] Showing UI (click box or press ESC to exit)");
    RunOverlaySmoke();   // visual smoke test

    std::puts("[done] All smoke checks completed.");
    return 0;
}
