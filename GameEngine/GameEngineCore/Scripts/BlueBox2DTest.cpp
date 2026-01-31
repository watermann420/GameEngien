#include "BlueBox2DTest.h"
#include "../RenderPipeline2D/Renderer2D.h"
#include "../textur/VideoRenderPipline/IFrameSource.h"
#include "../textur/VideoRenderPipline/VideoTexture.h"
#include "../textur/VideoRenderPipline/TexturePool.h"
#include "../Functionality/Text/TextLabel.h"
#include "../Physics/Physics2D/PhysicsWorld2D.h"
#include "../Physics/Physics3D/PhysicsWorld3D.h"
#include <vector>
#include <memory>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace
{
    TextLabel g_label(L"GameEngine MEL", 40, RGB(255, 255, 255), 2, 3, RGB(0, 0, 0));
    float g_hue = 0.0f;
    const float kHueSpeed = 48.0f; // degrees per second

    COLORREF HueToRgb(float h)
    {
        float s = 1.0f, v = 1.0f;
        float c = v * s;
        float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        float r, g, b;
        if (h < 60) { r = c; g = x; b = 0; }
        else if (h < 120) { r = x; g = c; b = 0; }
        else if (h < 180) { r = 0; g = c; b = x; }
        else if (h < 240) { r = 0; g = x; b = c; }
        else if (h < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        auto toByte = [m](float f)
        {
            float v2 = (f + m) * 255.0f;
            if (v2 < 0.0f) v2 = 0.0f;
            if (v2 > 255.0f) v2 = 255.0f;
            return static_cast<BYTE>(v2);
        };
        return RGB(toByte(r), toByte(g), toByte(b));
    }
}

void InitBlueBoxText(const RECT& bounds)
{
    g_label.SetPosition(static_cast<float>(bounds.right - 200), static_cast<float>((bounds.bottom - bounds.top) / 3));
    g_label.SetVelocity(-180.0f, 140.0f);
}

void UpdateBlueBoxText(double dt, HDC hdc, const RECT& bounds)
{
    g_hue = fmodf(g_hue + kHueSpeed * static_cast<float>(dt), 360.0f);
    COLORREF base = HueToRgb(g_hue);
    g_label.SetBaseColor(base);
    g_label.SetGlow(3, RGB(10, 10, 10));

    const size_t rainbowLen = g_label.Length();
    if (rainbowLen > 0)
    {
        std::vector<COLORREF> colors;
        colors.reserve(rainbowLen);
        for (size_t i = 0; i < rainbowLen; ++i)
        {
            float h2 = fmodf(g_hue + static_cast<float>(i) * 18.0f, 360.0f);
            colors.push_back(HueToRgb(h2));
        }
        g_label.SetCharColors(colors);
    }

    g_label.Update(dt, bounds);
    g_label.Draw(hdc);
}

// Minimal test scene as if a user was scripting the engine.
int RunBlueBox2DTest()
{
    // Physics sanity
    PhysicsWorld2D world2d;
    auto body = world2d.CreateBody();
    body->SetMass(1.0);
    world2d.Step(1.0 / 60.0);

    // Renderer-driven frame source for VideoTexture
    class BoxFrameSource : public IFrameSource
    {
    public:
        BoxFrameSource(uint32_t w, uint32_t h) : m_renderer(200, 200, RGB(0, 122, 255))
        {
            m_desc.width = w; m_desc.height = h; m_desc.stride = w * 4;
        }
        TextureDesc Describe() const override { return m_desc; }
        bool TryReadFrame(std::vector<uint8_t>& buffer, uint64_t& timestampNs) override
        {
            m_renderer.RenderToBuffer(static_cast<int>(m_desc.width), static_cast<int>(m_desc.height), buffer);
            timestampNs += 16'666'667;
            return true;
        }
    private:
        TextureDesc m_desc{};
        Renderer2D m_renderer;
    };

    // Headless validation: render a few frames with box + text to BMP.
    const int w = 800;
    const int h = 600;

    HDC screen = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, w, h);
    HGDIOBJ oldBmp = SelectObject(memDC, bmp);

    Renderer2D boxRenderer(200, 200, RGB(0, 122, 255));
    RECT bounds{ 0, 0, w, h };
    InitBlueBoxText(bounds);

    const double dt = 1.0 / 60.0;
    const int frames = 120; // shorter run
    for (int i = 0; i < frames; ++i)
    {
        HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(memDC, &bounds, bg);
        DeleteObject(bg);

        boxRenderer.RenderToDC(memDC, bounds);
        UpdateBlueBoxText(dt, memDC, bounds);
    }

    // Save final frame to disk for inspection.
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);
    GetDIBits(memDC, bmp, 0, h, pixels.data(), &bmi, DIB_RGB_COLORS);

    BITMAPFILEHEADER bfh{};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + static_cast<DWORD>(pixels.size());

    HANDLE hFile = CreateFileW(L"BlueBox2DTest_output.bmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteFile(hFile, &bfh, sizeof(bfh), &written, nullptr);
        WriteFile(hFile, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER), &written, nullptr);
        WriteFile(hFile, pixels.data(), static_cast<DWORD>(pixels.size()), &written, nullptr);
        CloseHandle(hFile);
        std::cout << "BlueBox2DTest: saved BlueBox2DTest_output.bmp with box + bouncing text\n";
    }
    else
    {
        std::cout << "BlueBox2DTest: failed to write BlueBox2DTest_output.bmp\n";
    }

    SelectObject(memDC, oldBmp);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screen);

    return 0;
}
