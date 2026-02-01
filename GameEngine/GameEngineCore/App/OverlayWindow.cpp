#include "OverlayWindow.h"

#include "AppState.h"
#include "AudioPlayback.h"

#include <windowsx.h>

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (g_app.collider.Contains(p))
        {
            PlayMusicEngineNote();
        }
        return 0;
    }
    case WM_RBUTTONDOWN:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

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

void InitOverlayBuffers(HDC screen)
{
    g_app.memDC = CreateCompatibleDC(screen);
    g_app.memBmp = CreateCompatibleBitmap(screen, g_app.bounds.right, g_app.bounds.bottom);
    g_app.oldBmp = SelectObject(g_app.memDC, g_app.memBmp);
}

void CleanupOverlayBuffers()
{
    if (g_app.oldBmp) SelectObject(g_app.memDC, g_app.oldBmp);
    if (g_app.memBmp) DeleteObject(g_app.memBmp);
    if (g_app.memDC) DeleteDC(g_app.memDC);
    g_app.memDC = nullptr;
    g_app.memBmp = nullptr;
    g_app.oldBmp = nullptr;
}

void UpdateOverlay(HDC screen)
{
    POINT pos{ 0,0 };
    SIZE size{ g_app.bounds.right, g_app.bounds.bottom };
    POINT src{ 0,0 };
    COLORREF key = RGB(255, 0, 255);
    UpdateLayeredWindow(g_app.overlay, screen, &pos, &size, g_app.memDC, &src, key, nullptr, ULW_COLORKEY);
}
