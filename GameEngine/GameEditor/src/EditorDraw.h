#pragma once

#include <windows.h>

void DrawHeader(HDC hdc, const RECT& rect, const wchar_t* label);
void DrawScenePreview(HDC hdc,
    const RECT& rect,
    bool playing,
    float zoom,
    int panX,
    int panY,
    bool view3D,
    float camYaw,
    float camPitch,
    float camX,
    float camY,
    float camZ,
    bool showCenterCube,
    bool showOrbiter);
void DrawInspector(HDC hdc, const RECT& rect, bool playing);
