#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "VideoTexture.h"

// Shared cache to ensure identical textures/videos are created once and reused.
class TextureCache
{
public:
    using Factory = std::function<std::shared_ptr<VideoTexture>()>;

    // Returns an existing texture for the key or constructs one via factory.
    std::shared_ptr<VideoTexture> GetOrCreate(const std::string& key, const Factory& factory);

    // Drops all cached entries. Callers retain their shared_ptrs.
    void Clear();

private:
    std::mutex m_mutex;
    std::unordered_map<std::string, std::weak_ptr<VideoTexture>> m_cache;
};

