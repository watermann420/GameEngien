#include "TexturePool.h"

TexturePool::TexturePool(size_t bytesPerBuffer, size_t reserveCount)
    : m_bytesPerBuffer(bytesPerBuffer)
{
    m_free.reserve(reserveCount);
    for (size_t i = 0; i < reserveCount; ++i)
    {
        auto buf = std::make_shared<std::vector<uint8_t>>();
        buf->resize(m_bytesPerBuffer);
        m_free.push_back(std::move(buf));
    }
}

std::shared_ptr<std::vector<uint8_t>> TexturePool::Acquire()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_free.empty())
    {
        auto buffer = m_free.back();
        m_free.pop_back();
        return buffer;
    }

    auto buffer = std::make_shared<std::vector<uint8_t>>();
    buffer->resize(m_bytesPerBuffer);
    return buffer;
}

void TexturePool::Release(std::shared_ptr<std::vector<uint8_t>> buffer)
{
    if (!buffer) return;
    if (buffer->size() < m_bytesPerBuffer)
    {
        buffer->resize(m_bytesPerBuffer);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_free.push_back(std::move(buffer));
}

