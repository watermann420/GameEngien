#pragma once
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include "FrameTypes.h"
#include "IFrameSource.h"
#include "TexturePool.h"

// High-throughput video texture that keeps the latest decoded frame available with minimal locking.
class VideoTexture
{
public:
    VideoTexture(std::unique_ptr<IFrameSource> source, size_t poolReserve = 4, size_t slots = 3);
    ~VideoTexture();

    // Start background decoding/streaming.
    void Start();

    // Stop background work and release resources.
    void Stop();

    // Retrieves the most recent frame view if available. Returns false when no frame is ready yet.
    bool TryGetFrame(FrameView& view) const;

    TextureDesc Describe() const { return m_desc; }
    uint64_t FramesDecoded() const { return m_latestTicket.load(); }

private:
    void Worker();

    struct FrameSlot
    {
        std::shared_ptr<std::vector<uint8_t>> buffer;
        uint64_t timestamp{ 0 };
    };

    TextureDesc m_desc{};
    std::unique_ptr<IFrameSource> m_source;
    TexturePool m_pool;
    std::vector<FrameSlot> m_slots;

    std::atomic<bool> m_running{ false };
    std::thread m_thread;
    std::atomic<uint64_t> m_latestTicket{ 0 };
};

