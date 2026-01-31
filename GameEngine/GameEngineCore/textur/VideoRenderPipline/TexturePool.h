#pragma once
#include <memory>
#include <mutex>
#include <vector>

// Lightweight buffer pool to keep thousands of video frames alive without thrashing the allocator.
class TexturePool
{
public:
    TexturePool(size_t bytesPerBuffer, size_t reserveCount);

    // Grabs a buffer of at least bytesPerBuffer size. Thread-safe.
    std::shared_ptr<std::vector<uint8_t>> Acquire();

    // Returns a buffer to the pool. Thread-safe.
    void Release(std::shared_ptr<std::vector<uint8_t>> buffer);

    size_t BufferSize() const { return m_bytesPerBuffer; }

private:
    size_t m_bytesPerBuffer;
    std::mutex m_mutex;
    std::vector<std::shared_ptr<std::vector<uint8_t>>> m_free;
};

