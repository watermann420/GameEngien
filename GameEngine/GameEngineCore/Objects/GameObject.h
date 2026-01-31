#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "Component.h"

// Hierarchical scene object similar to Unity's GameObject, usable from C++.
class GameObject
{
public:
    explicit GameObject(std::string name = "GameObject", std::string tag = "");
    ~GameObject();

    // Hierarchy
    GameObject* Parent() const { return m_parent; }
    const std::vector<std::unique_ptr<GameObject>>& Children() const { return m_children; }
    GameObject* AddChild(std::unique_ptr<GameObject> child);
    GameObject* CreateChild(const std::string& name, const std::string& tag = "");
    GameObject* FindInChildrenByName(const std::string& name);

    // Components
    Component* AddComponent(std::unique_ptr<Component> component);
    template <typename T>
    T* AddComponent()
    {
        auto comp = std::make_unique<T>();
        T* raw = comp.get();
        AddComponent(std::move(comp));
        return raw;
    }

    template <typename T>
    T* GetComponent() const
    {
        for (const auto& c : m_components)
        {
            if (auto casted = dynamic_cast<T*>(c.get()))
                return casted;
        }
        return nullptr;
    }

    // Identity
    const std::string& Name() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    const std::string& Tag() const { return m_tag; }
    void SetTag(const std::string& tag) { m_tag = tag; }

    // Traversal / utility
    void ForEachChild(const std::function<void(GameObject*)>& visitor, bool recursive = true);
    void ForEachComponent(const std::function<void(Component*)>& visitor);

private:
    GameObject* m_parent{ nullptr };
    std::vector<std::unique_ptr<GameObject>> m_children;
    std::vector<std::unique_ptr<Component>> m_components;
    std::string m_name;
    std::string m_tag;
};

