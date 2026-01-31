#pragma once
#include <cstdint>
#include <vector>

class Renderer2D
{
public:
    Renderer2D(uint32_t boxWidth, uint32_t boxHeight, uint8_t r, uint8_t g, uint8_t b);

    uint32_t BoxWidth() const { return m_boxWidth; }
    uint32_t BoxHeight() const { return m_boxHeight; }

    // Render centered box into RGBA buffer (BGRA layout).
    void RenderToBuffer(uint32_t width, uint32_t height, std::vector<uint8_t>& outBGRA) const;

private:
    uint32_t m_boxWidth;
    uint32_t m_boxHeight;
    uint8_t m_r, m_g, m_b;
};

