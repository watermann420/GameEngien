#pragma once

#include <windows.h>

HWND CreateOverlayWindow(HINSTANCE hInstance, int width, int height);
void InitOverlayBuffers(HDC screen);
void CleanupOverlayBuffers();
void UpdateOverlay(HDC screen);
