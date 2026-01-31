#include "GameObject.h"

GameObject::GameObject(std::string name, std::string tag)
    : m_name(std::move(name)), m_tag(std::move(tag))
{
}

GameObject::~GameObject() = default;

GameObject* GameObject::AddChild(std::unique_ptr<GameObject> child)
{
    if (!child) return nullptr;
    child->m_parent = this;
    GameObject* raw = child.get();
    m_children.push_back(std::move(child));
    return raw;
}

GameObject* GameObject::CreateChild(const std::string& name, const std::string& tag)
{
    auto child = std::make_unique<GameObject>(name, tag);
    return AddChild(std::move(child));
}

GameObject* GameObject::FindInChildrenByName(const std::string& name)
{
    for (auto& c : m_children)
    {
        if (c->Name() == name) return c.get();
        if (auto* found = c->FindInChildrenByName(name)) return found;
    }
    return nullptr;
}

Component* GameObject::AddComponent(std::unique_ptr<Component> component)
{
    if (!component) return nullptr;
    component->SetOwner(this);
    Component* raw = component.get();
    m_components.push_back(std::move(component));
    return raw;
}

void GameObject::ForEachChild(const std::function<void(GameObject*)>& visitor, bool recursive)
{
    for (auto& c : m_children)
    {
        visitor(c.get());
        if (recursive) c->ForEachChild(visitor, true);
    }
}

void GameObject::ForEachComponent(const std::function<void(Component*)>& visitor)
{
    for (auto& c : m_components) visitor(c.get());
}

