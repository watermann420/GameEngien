#include "VideoPlane.h"

void VideoPlane::InitFromFrame(const TextureDesc& desc, const RECT& screenBounds, float scale)
{
    int screenW = screenBounds.right - screenBounds.left;
    w = static_cast<int>(screenW * scale);
    h = static_cast<int>(w * (static_cast<float>(desc.height) / desc.width));
    x = (screenW - w) * 0.7f;
    y = (screenBounds.bottom - screenBounds.top - h) * 0.3f;
    initialized = true;
}

void VideoPlane::Update(double dt, const RECT& screenBounds)
{
    if (!initialized) return;
    x += vx * static_cast<float>(dt);
    y += vy * static_cast<float>(dt);
    float maxX = static_cast<float>(screenBounds.right - screenBounds.left - w);
    float maxY = static_cast<float>(screenBounds.bottom - screenBounds.top - h);
    if (x < 0) { x = 0; vx = -vx; }
    if (y < 0) { y = 0; vy = -vy; }
    if (x > maxX) { x = maxX; vx = -vx; }
    if (y > maxY) { y = maxY; vy = -vy; }
}

void VideoPlane::Render(HDC hdc, const FrameView& view)
{
    if (!initialized) return;
    BITMAPINFO bmi = MakeBitmapInfo(view.desc);
    StretchDIBits(
        hdc,
        static_cast<int>(x), static_cast<int>(y), w, h,
        0, 0, static_cast<int>(view.desc.width), static_cast<int>(view.desc.height),
        view.data,
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY);
}

RECT VideoPlane::Bounds() const
{
    return RECT{ static_cast<int>(x), static_cast<int>(y), static_cast<int>(x) + w, static_cast<int>(y) + h };
}
