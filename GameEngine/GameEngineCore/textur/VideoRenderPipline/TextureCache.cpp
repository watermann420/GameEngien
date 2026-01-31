#include "TextureCache.h"

std::shared_ptr<VideoTexture> TextureCache::GetOrCreate(const std::string& key, const Factory& factory)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto existing = m_cache[key].lock())
    {
        return existing;
    }

    auto created = factory ? factory() : nullptr;
    if (created)
    {
        m_cache[key] = created;
    }
    return created;
}

void TextureCache::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
}

