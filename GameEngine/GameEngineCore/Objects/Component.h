#pragma once
#include <memory>

class GameObject;

// Unity-like lightweight component base. Extend this for gameplay logic.
class Component
{
public:
    virtual ~Component() = default;

    GameObject* Owner() const { return m_owner; }

    // Called every frame; engine/user can invoke manually.
    virtual void OnUpdate(double /*dt*/) {}

private:
    friend class GameObject;
    void SetOwner(GameObject* owner) { m_owner = owner; }

    GameObject* m_owner{ nullptr };
};

