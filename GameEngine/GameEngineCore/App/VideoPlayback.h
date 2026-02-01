#pragma once

#include <windows.h>

bool InitVideoFromFiles();
bool UpdateVideoAndRender(double dt, HDC targetDC);
void RenderFallback(HDC targetDC);
