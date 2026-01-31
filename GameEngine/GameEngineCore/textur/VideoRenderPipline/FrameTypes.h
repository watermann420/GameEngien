#pragma once
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#endif
#include <cstddef>

// Lightweight texture description used across the video render pipeline.
enum class TextureFormat
{
    BGRA8 // 4 bytes per pixel, byte order compatible with GDI DIB sections
};

struct TextureDesc
{
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    TextureFormat format{ TextureFormat::BGRA8 };
    uint32_t stride{ 0 }; // bytes per row

    uint32_t BytesPerFrame() const { return stride * height; }
};

// Simple view onto a texture buffer. Ownership is managed elsewhere.
struct FrameView
{
    uint8_t* data{ nullptr };
    uint32_t stride{ 0 };
    TextureDesc desc{};
    uint64_t timestampNs{ 0 };

    bool Empty() const { return data == nullptr; }
};

#ifdef _WIN32
// Helper: construct a BITMAPINFO matching our TextureDesc for fast StretchDIBits blits (Win32 only).
inline BITMAPINFO MakeBitmapInfo(const TextureDesc& desc)
{
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(desc.width);
    bmi.bmiHeader.biHeight = -static_cast<LONG>(desc.height); // top-down for zero-copy blits
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    return bmi;
}
#endif

