#include "ObjectAPI.h"
#include "GameObject.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>

namespace
{
    std::atomic<uint64_t> g_nextId{ 1 };
    std::unordered_map<uint64_t, std::unique_ptr<GameObject>> g_roots;
    std::mutex g_mutex;

    GameObject* Resolve(uint64_t id)
    {
        auto it = g_roots.find(id);
        if (it != g_roots.end()) return it->second.get();
        return nullptr;
    }
}

uint64_t GE_CreateObject(const char* name, const char* tag)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    const uint64_t id = g_nextId++;
    g_roots[id] = std::make_unique<GameObject>(name ? name : "GameObject", tag ? tag : "");
    return id;
}

void GE_DestroyObject(uint64_t id)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_roots.erase(id);
}

uint64_t GE_AddChild(uint64_t parentId, const char* name, const char* tag)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto* parent = Resolve(parentId);
    if (!parent) return 0;
    auto child = std::make_unique<GameObject>(name ? name : "GameObject", tag ? tag : "");
    GameObject* added = parent->AddChild(std::move(child));
    // Expose children only through parent; they don't get global IDs for simplicity.
    return reinterpret_cast<uint64_t>(added);
}

uint32_t GE_GetChildCount(uint64_t parentId)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto* parent = Resolve(parentId);
    if (!parent) return 0;
    return static_cast<uint32_t>(parent->Children().size());
}

uint64_t GE_GetChildAt(uint64_t parentId, uint32_t index)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto* parent = Resolve(parentId);
    if (!parent) return 0;
    if (index >= parent->Children().size()) return 0;
    return reinterpret_cast<uint64_t>(parent->Children()[index].get());
}

uint64_t GE_FindByName(const char* name)
{
    if (!name) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& kv : g_roots)
    {
        if (kv.second->Name() == name) return kv.first;
        if (auto* found = kv.second->FindInChildrenByName(name))
        {
            return reinterpret_cast<uint64_t>(found);
        }
    }
    return 0;
}

void GE_SetName(uint64_t id, const char* name)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (auto* go = Resolve(id)) go->SetName(name ? name : "");
}

const char* GE_GetName(uint64_t id)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (auto* go = Resolve(id)) return go->Name().c_str();
    return "";
}

void GE_SetTag(uint64_t id, const char* tag)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (auto* go = Resolve(id)) go->SetTag(tag ? tag : "");
}

const char* GE_GetTag(uint64_t id)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (auto* go = Resolve(id)) return go->Tag().c_str();
    return "";
}

