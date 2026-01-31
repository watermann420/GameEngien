#include "VideoTexture.h"
#include <chrono>
#include <utility>

namespace
{
    TextureDesc Normalize(TextureDesc desc)
    {
        if (desc.stride == 0 && desc.width > 0)
        {
            desc.stride = desc.width * 4; // default for BGRA8
        }
        return desc;
    }
}

VideoTexture::VideoTexture(std::unique_ptr<IFrameSource> source, size_t poolReserve, size_t slots)
    : m_desc(Normalize(source ? source->Describe() : TextureDesc{})),
      m_source(std::move(source)),
      m_pool(m_desc.stride * m_desc.height, poolReserve),
      m_slots(slots)
{
}

VideoTexture::~VideoTexture()
{
    Stop();
}

void VideoTexture::Start()
{
    if (m_running.exchange(true)) return;
    m_thread = std::thread(&VideoTexture::Worker, this);
}

void VideoTexture::Stop()
{
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

bool VideoTexture::TryGetFrame(FrameView& view) const
{
    const uint64_t ticket = m_latestTicket.load(std::memory_order_acquire);
    if (ticket == 0 || m_slots.empty())
    {
        return false;
    }

    const size_t index = ticket % m_slots.size();
    const auto& slot = m_slots[index];
    if (!slot.buffer || slot.buffer->empty()) return false;

    view.data = slot.buffer->data();
    view.stride = m_desc.stride;
    view.desc = m_desc;
    view.timestampNs = slot.timestamp;
    return true;
}

void VideoTexture::Worker()
{
    using namespace std::chrono_literals;
    uint64_t ticket = 0;

    while (m_running.load(std::memory_order_relaxed))
    {
        if (!m_source)
        {
            std::this_thread::sleep_for(2ms);
            continue;
        }

        auto buffer = m_pool.Acquire();
        uint64_t ts = 0;
        if (!m_source->TryReadFrame(*buffer, ts))
        {
            m_pool.Release(std::move(buffer));
            std::this_thread::sleep_for(2ms);
            continue;
        }

        // Guard against undersized buffers if source changed description.
        if (buffer->size() < m_desc.BytesPerFrame())
        {
            buffer->resize(m_desc.BytesPerFrame());
        }

        ++ticket;
        const size_t index = ticket % m_slots.size();
        m_slots[index].buffer = std::move(buffer);
        m_slots[index].timestamp = ts;
        m_latestTicket.store(ticket, std::memory_order_release);
    }
}
