#include "BmpWriter.h"
#include <fstream>
#include <cstring>

#pragma pack(push, 1)
struct BmpFileHeader
{
    uint16_t bfType{ 0x4D42 }; // 'BM'
    uint32_t bfSize{ 0 };
    uint16_t bfReserved1{ 0 };
    uint16_t bfReserved2{ 0 };
    uint32_t bfOffBits{ 54 };
};

struct BmpInfoHeader
{
    uint32_t biSize{ 40 };
    int32_t  biWidth{ 0 };
    int32_t  biHeight{ 0 };
    uint16_t biPlanes{ 1 };
    uint16_t biBitCount{ 32 };
    uint32_t biCompression{ 0 }; // BI_RGB
    uint32_t biSizeImage{ 0 };
    int32_t  biXPelsPerMeter{ 0 };
    int32_t  biYPelsPerMeter{ 0 };
    uint32_t biClrUsed{ 0 };
    uint32_t biClrImportant{ 0 };
};
#pragma pack(pop)

bool WriteBmpBGRA(const std::string& path, uint32_t width, uint32_t height, const std::vector<uint8_t>& data)
{
    if (width == 0 || height == 0) return false;
    const size_t expected = static_cast<size_t>(width) * height * 4;
    if (data.size() < expected) return false;

    BmpFileHeader fh{};
    BmpInfoHeader ih{};
    ih.biWidth = static_cast<int32_t>(width);
    ih.biHeight = -static_cast<int32_t>(height); // top-down
    ih.biSizeImage = static_cast<uint32_t>(expected);
    fh.bfOffBits = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);
    fh.bfSize = fh.bfOffBits + ih.biSizeImage;

    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    out.write(reinterpret_cast<const char*>(&ih), sizeof(ih));
    out.write(reinterpret_cast<const char*>(data.data()), expected);
    return out.good();
}

