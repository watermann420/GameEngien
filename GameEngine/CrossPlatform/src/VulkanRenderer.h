#pragma once
#include <cstdint>
#include <vector>
#include <string>

class VulkanRenderer
{
public:
    // Attempts to initialize Vulkan + (optional) SDL2 window; falls back to headless if not available.
    bool Init(uint32_t width, uint32_t height, const std::string& title);
    void Shutdown();

    // Render a single clear frame (for smoke test).
    bool RenderOnce(float r, float g, float b);

    // Copy a BGRA8 frame (width x height) into the swapchain (no shaders required).
    bool RenderBGRAFrame(uint32_t width, uint32_t height, const uint8_t* data);

private:
    // Opaque handles (forward-declared to avoid heavy includes when Vulkan missing).
    struct Impl;
    Impl* m_impl{ nullptr };
};
