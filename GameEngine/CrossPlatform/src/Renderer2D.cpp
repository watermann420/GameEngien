#include "Renderer2D.h"
#include <algorithm>

Renderer2D::Renderer2D(uint32_t boxWidth, uint32_t boxHeight, uint8_t r, uint8_t g, uint8_t b)
    : m_boxWidth(boxWidth), m_boxHeight(boxHeight), m_r(r), m_g(g), m_b(b)
{
}

void Renderer2D::RenderToBuffer(uint32_t width, uint32_t height, std::vector<uint8_t>& outBGRA) const
{
    outBGRA.resize(static_cast<size_t>(width) * height * 4, 255);
    const int cx = static_cast<int>(width) / 2;
    const int cy = static_cast<int>(height) / 2;
    const int halfW = static_cast<int>(m_boxWidth) / 2;
    const int halfH = static_cast<int>(m_boxHeight) / 2;

    const int left = std::max(0, cx - halfW);
    const int right = std::min(static_cast<int>(width), cx + halfW);
    const int top = std::max(0, cy - halfH);
    const int bottom = std::min(static_cast<int>(height), cy + halfH);

    for (int y = top; y < bottom; ++y)
    {
        for (int x = left; x < right; ++x)
        {
            const size_t idx = (static_cast<size_t>(y) * width + x) * 4;
            outBGRA[idx + 0] = m_b;
            outBGRA[idx + 1] = m_g;
            outBGRA[idx + 2] = m_r;
            outBGRA[idx + 3] = 255;
        }
    }
}

