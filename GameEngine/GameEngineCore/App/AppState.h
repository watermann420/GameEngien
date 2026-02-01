#pragma once

#include <windows.h>
#include <mmreg.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../RenderPipeline2D/Renderer2D.h"
#include "../Colliders/BoxCollider2D.h"
#include "../Functionality/Video/VideoPlane.h"
#include "../textur/VideoRenderPipline/VideoTexture.h"

struct AppState
{
    static constexpr int kWindowWidth = 800;
    static constexpr int kWindowHeight = 600;

    Renderer2D renderer{ 200, 200, RGB(0, 122, 255) };
    BoxCollider2D collider{};
    HWND overlay = nullptr;
    HMIDIOUT midiOut = nullptr;

    HDC memDC = nullptr;
    HBITMAP memBmp = nullptr;
    HGDIOBJ oldBmp = nullptr;
    RECT bounds{};

    std::unique_ptr<VideoTexture> videoTexture;
    bool videoAudioStarted = false;
    std::wstring videoPath;
    std::wstring audioPath;
    VideoPlane plane{};
    std::vector<VideoPlane> planes;
    double spawnAccumulator = 0.0;
    std::vector<BYTE> audioWavBytes;
    bool audioOpen = false;
    std::vector<BYTE> audioWav;
    std::wstring projectPath;
};

extern AppState g_app;
