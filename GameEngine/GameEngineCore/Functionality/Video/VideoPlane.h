#pragma once
#include <windows.h>
#include <cstdint>
#include "../../textur/VideoRenderPipline/FrameTypes.h"

class VideoPlane
{
public:
    void InitFromFrame(const TextureDesc& desc, const RECT& screenBounds, float scale = 0.4f);
    void Update(double dt, const RECT& screenBounds);
    void Render(HDC hdc, const FrameView& view);
    RECT Bounds() const;
    void SetVelocity(float vx_, float vy_) { vx = vx_; vy = vy_; }
    void SetPosition(float x_, float y_) { x = x_; y = y_; }
    bool Initialized() const { return initialized; }

private:
    int w{ 0 }, h{ 0 };
    float x{ 0 }, y{ 0 };
    float vx{ -180.0f }, vy{ 140.0f };
    bool initialized{ false };
};
