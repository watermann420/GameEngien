#pragma once
#include <memory>
#include <vector>
#include <windows.h>
#include <string>
#include <functional>
#include "VideoTexture.h"
#include "TextureCache.h"

// Coordinates many VideoTexture instances and provides fast blitting into a target HDC.
class VideoRenderPipeline
{
public:
    VideoRenderPipeline() = default;

    // Create a synthetic high-FPS stream for diagnostics and throughput testing.
    size_t AddSyntheticStream(uint32_t width, uint32_t height, double fps = 30.0);

    // Register a custom frame source. Ownership of the source is transferred.
    size_t AddStream(std::unique_ptr<IFrameSource> source);

    // Register a shared stream that is cached by key. Only one instance per key is created.
    // The factory is invoked only when the key is missing.
    size_t AddCachedStream(const std::string& key, std::function<std::unique_ptr<IFrameSource>()> factory);

    // Start decoding for every registered stream.
    void StartAll();

    // Stop all streams (joins threads).
    void StopAll();

    // Blit all streams in a simple grid into the provided device context.
    void BlitAllToDC(HDC hdc, const RECT& viewport);

    const std::vector<std::shared_ptr<VideoTexture>>& Streams() const { return m_streams; }

private:
    TextureCache m_cache;
    std::vector<std::shared_ptr<VideoTexture>> m_streams;
};
