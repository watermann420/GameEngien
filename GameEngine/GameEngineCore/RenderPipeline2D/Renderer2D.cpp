#include "Renderer2D.h"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

Renderer2D::Renderer2D(int boxWidth, int boxHeight, COLORREF color)
    : m_boxWidth(boxWidth), m_boxHeight(boxHeight), m_color(color)
{
}

void Renderer2D::RenderToDC(HDC hdc, const RECT& area) const
{
    int centerX = (area.right - area.left) / 2;
    int centerY = (area.bottom - area.top) / 2;

    RECT box{
        centerX - m_boxWidth / 2,
        centerY - m_boxHeight / 2,
        centerX + m_boxWidth / 2,
        centerY + m_boxHeight / 2
    };

    HBRUSH brush = CreateSolidBrush(m_color);
    FillRect(hdc, &box, brush);
    DeleteObject(brush);
}

void Renderer2D::RenderToBuffer(int width, int height, std::vector<uint8_t>& out) const
{
    // Top-down BGRA
    out.assign(static_cast<size_t>(width) * height * 4, 255);

    int centerX = width / 2;
    int centerY = height / 2;

    int left = centerX - m_boxWidth / 2;
    int right = centerX + m_boxWidth / 2;
    int top = centerY - m_boxHeight / 2;
    int bottom = centerY + m_boxHeight / 2;

    left = (std::max)(0, left);
    right = (std::min)(width, right);
    top = (std::max)(0, top);
    bottom = (std::min)(height, bottom);

    const uint8_t b = GetBValue(m_color);
    const uint8_t g = GetGValue(m_color);
    const uint8_t r = GetRValue(m_color);

    for (int y = top; y < bottom; ++y)
    {
        size_t row = static_cast<size_t>(y) * width * 4;
        for (int x = left; x < right; ++x)
        {
            size_t idx = row + x * 4;
            out[idx + 0] = b;
            out[idx + 1] = g;
            out[idx + 2] = r;
            out[idx + 3] = 255;
        }
    }
}

bool Renderer2D::RenderToBitmapFile(const wchar_t* filePath, int width, int height) const
{
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits)
    {
        if (dib) DeleteObject(dib);
        if (memDC) DeleteDC(memDC);
        if (screenDC) ReleaseDC(nullptr, screenDC);
        return false;
    }

    HGDIOBJ oldBmp = SelectObject(memDC, dib);

    RECT area{ 0,0,width,height };
    HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(memDC, &area, bg);
    DeleteObject(bg);

    RenderToDC(memDC, area);

    SelectObject(memDC, oldBmp);

    // Prepare BITMAPFILEHEADER + BITMAPINFOHEADER + pixel data
    const DWORD pixelDataSize = width * height * 4;
    BITMAPFILEHEADER bfh{};
    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + pixelDataSize;

    HANDLE hFile = CreateFileW(filePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DeleteObject(dib);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return false;
    }

    DWORD written = 0;
    bool ok = true;
    ok &= WriteFile(hFile, &bfh, sizeof(bfh), &written, nullptr) && written == sizeof(bfh);
    written = 0;
    ok &= WriteFile(hFile, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER), &written, nullptr) && written == sizeof(BITMAPINFOHEADER);
    if (ok)
    {
        written = 0;
        ok &= WriteFile(hFile, bits, pixelDataSize, &written, nullptr) && written == pixelDataSize;
    }

    CloseHandle(hFile);
    DeleteObject(dib);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    return ok;
}
