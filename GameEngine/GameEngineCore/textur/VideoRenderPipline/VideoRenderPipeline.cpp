#include "VideoRenderPipeline.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include "FrameTypes.h"

namespace
{
    // Minimal synthetic frame generator to stress-test the pipeline without external codecs.
    class SyntheticFrameSource final : public IFrameSource
    {
    public:
        SyntheticFrameSource(uint32_t w, uint32_t h, double fps)
            : m_desc{ w, h, TextureFormat::BGRA8, w * 4 },
              m_frameInterval(std::chrono::duration<double>(1.0 / (std::max)(1.0, fps)))
        {
            m_started = std::chrono::steady_clock::now();
            m_nextFrame = m_started;
        }

        TextureDesc Describe() const override { return m_desc; }

        bool TryReadFrame(std::vector<uint8_t>& buffer, uint64_t& timestampNs) override
        {
            auto now = std::chrono::steady_clock::now();
            if (now < m_nextFrame)
            {
                std::this_thread::sleep_for(m_nextFrame - now);
            }
            m_nextFrame += std::chrono::duration_cast<std::chrono::steady_clock::duration>(m_frameInterval);

            const size_t required = m_desc.BytesPerFrame();
            if (buffer.size() < required) buffer.resize(required);

            // Generate a cheap moving gradient; stride is guaranteed contiguous.
            const uint32_t pitch = m_desc.stride;
            const uint32_t w = m_desc.width;
            const uint32_t h = m_desc.height;
            const uint32_t frameId = ++m_frameCounter;

            for (uint32_t y = 0; y < h; ++y)
            {
                uint8_t* row = buffer.data() + y * pitch;
                for (uint32_t x = 0; x < w; ++x)
                {
                    row[x * 4 + 0] = static_cast<uint8_t>((x + frameId) & 0xFF);          // B
                    row[x * 4 + 1] = static_cast<uint8_t>((y + frameId * 2) & 0xFF);      // G
                    row[x * 4 + 2] = static_cast<uint8_t>((x + y + frameId * 3) & 0xFF);  // R
                    row[x * 4 + 3] = 255; // A
                }
            }

            timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(m_nextFrame - m_started).count();
            return true;
        }

    private:
        TextureDesc m_desc{};
        std::chrono::steady_clock::time_point m_started;
        std::chrono::steady_clock::time_point m_nextFrame;
        std::chrono::duration<double> m_frameInterval;
        uint64_t m_frameCounter{ 0 };
    };
}

size_t VideoRenderPipeline::AddSyntheticStream(uint32_t width, uint32_t height, double fps)
{
    auto source = std::make_unique<SyntheticFrameSource>(width, height, fps);
    return AddStream(std::move(source));
}

size_t VideoRenderPipeline::AddStream(std::unique_ptr<IFrameSource> source)
{
    auto texture = std::make_shared<VideoTexture>(std::move(source));
    m_streams.push_back(texture);
    return m_streams.size() - 1;
}

size_t VideoRenderPipeline::AddCachedStream(const std::string& key, std::function<std::unique_ptr<IFrameSource>()> factory)
{
    auto texture = m_cache.GetOrCreate(key, [&]() -> std::shared_ptr<VideoTexture>
        {
            return std::make_shared<VideoTexture>(factory ? factory() : nullptr);
        });
    if (texture)
    {
        m_streams.push_back(texture);
        return m_streams.size() - 1;
    }
    return static_cast<size_t>(-1);
}

void VideoRenderPipeline::StartAll()
{
    for (auto& s : m_streams) if (s) s->Start();
}

void VideoRenderPipeline::StopAll()
{
    for (auto& s : m_streams) if (s) s->Stop();
}

void VideoRenderPipeline::BlitAllToDC(HDC hdc, const RECT& viewport)
{
    if (m_streams.empty()) return;

    const size_t count = m_streams.size();
    const size_t cols = static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(count))));
    const size_t rows = static_cast<size_t>(std::ceil(static_cast<double>(count) / cols));

    const int vw = static_cast<int>(viewport.right) - static_cast<int>(viewport.left);
    const int vh = static_cast<int>(viewport.bottom) - static_cast<int>(viewport.top);
    const int cellW = (std::max)(1, vw / static_cast<int>(cols));
    const int cellH = (std::max)(1, vh / static_cast<int>(rows));

    size_t idx = 0;
    for (size_t r = 0; r < rows; ++r)
    {
        for (size_t c = 0; c < cols && idx < count; ++c, ++idx)
        {
            FrameView view{};
            if (!m_streams[idx] || !m_streams[idx]->TryGetFrame(view)) continue;

            BITMAPINFO bmi = MakeBitmapInfo(view.desc);
            const int x = viewport.left + static_cast<int>(c) * cellW;
            const int y = viewport.top + static_cast<int>(r) * cellH;

            StretchDIBits(
                hdc,
                x, y, cellW, cellH,
                0, 0, static_cast<int>(view.desc.width), static_cast<int>(view.desc.height),
                view.data,
                &bmi,
                DIB_RGB_COLORS,
                SRCCOPY);
        }
    }
}
