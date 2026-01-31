#include <windows.h>
#include <windowsx.h>
#include "RenderPipeline2D/Renderer2D.h"
#include "Colliders/BoxCollider2D.h"

static const int kWindowWidth = 800;
static const int kWindowHeight = 600;
static Renderer2D g_renderer(200, 200, RGB(0, 122, 255));
static BoxCollider2D g_collider;
static HWND g_overlay = nullptr;

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // client coords
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

// Creates a borderless layered overlay window to host the box without flicker.
HWND CreateOverlayWindow(HINSTANCE hInstance, int width, int height)
{
    const wchar_t* CLASS_NAME = L"GameEngineOverlayClass";

    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        CLASS_NAME,
        L"",
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

    // collider in client coords (same as box since window is box-sized)
    g_collider.Set(0, 0, width, height);

    SelectObject(memDC, old);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screen);

    ShowWindow(hwnd, SW_SHOW);
}

int main()
{
    SetProcessDPIAware(); // avoid DPI scaling shifts when centering on desktop

    // Headless/offscreen test: render into a BMP on disk
    g_renderer.RenderToBitmapFile(L"headless_output.bmp", kWindowWidth, kWindowHeight);

    // Flicker-free overlay: borderless layered window, topmost; click inside box to exit.
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    g_overlay = CreateOverlayWindow(hInstance, g_renderer.BoxWidth(), g_renderer.BoxHeight());
    ShowBoxOverlay(g_overlay, g_renderer.BoxWidth(), g_renderer.BoxHeight(), g_renderer.Color());

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(g_overlay);

    return 0;
}
