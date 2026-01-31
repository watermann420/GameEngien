#include "Renderer2D.h"
#include "BmpWriter.h"
#include <iostream>

int main()
{
    const uint32_t width = 800;
    const uint32_t height = 600;
    Renderer2D renderer(200, 200, 0, 122, 255); // blue box

    std::vector<uint8_t> buffer;
    renderer.RenderToBuffer(width, height, buffer);
    if (WriteBmpBGRA("headless_output_linux.bmp", width, height, buffer))
    {
        std::cout << "Rendered headless_output_linux.bmp\n";
        return 0;
    }

    std::cerr << "Failed to render BMP\n";
    return 1;
}

