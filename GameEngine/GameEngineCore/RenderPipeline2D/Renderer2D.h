#pragma once
#include <windows.h>

// Minimal 2D renderer that can draw a centered filled rectangle
// into any device context or offscreen bitmap.
class Renderer2D
{
public:
    Renderer2D(int boxWidth, int boxHeight, COLORREF color);
    int BoxWidth() const { return m_boxWidth; }
    int BoxHeight() const { return m_boxHeight; }
    COLORREF Color() const { return m_color; }

    // Render centered box into an existing device context and area
    void RenderToDC(HDC hdc, const RECT& area) const;

    // Headless render: draw into an offscreen DIB and save as BMP.
    // Returns true on success.
    bool RenderToBitmapFile(const wchar_t* filePath, int width, int height) const;

private:
    int m_boxWidth;
    int m_boxHeight;
    COLORREF m_color;
};
