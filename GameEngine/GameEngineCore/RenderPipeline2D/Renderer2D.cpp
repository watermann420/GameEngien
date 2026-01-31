#include "Renderer2D.h"
#include <vector>
#include <memory>
#include <string>

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
