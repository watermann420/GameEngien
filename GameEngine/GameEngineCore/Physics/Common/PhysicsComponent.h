#pragma once
#include <memory>
#include <variant>
#include "../../Objects/Component.h"
#include "PhysicsWorld.h"
#include "../Physics2D/RigidBody2D.h"
#include "../Physics3D/RigidBody3D.h"

// Component that links a GameObject to a physics world/body placeholder.
class PhysicsComponent : public Component
{
public:
    enum class Dimension { Dim2D, Dim3D };

    explicit PhysicsComponent(Dimension dim) : m_dim(dim) {}
    Dimension Dim() const { return m_dim; }

    void SetWorld(const PhysicsWorldPtr& world) { m_world = world; }
    PhysicsWorldPtr World() const { return m_world; }

    void SetBody(const RigidBody2D::Ptr& body) { m_body = body; }
    void SetBody(const RigidBody3D::Ptr& body) { m_body = body; }

    RigidBody2D::Ptr Body2D() const { return std::holds_alternative<RigidBody2D::Ptr>(m_body) ? std::get<RigidBody2D::Ptr>(m_body) : nullptr; }
    RigidBody3D::Ptr Body3D() const { return std::holds_alternative<RigidBody3D::Ptr>(m_body) ? std::get<RigidBody3D::Ptr>(m_body) : nullptr; }

private:
    Dimension m_dim;
    PhysicsWorldPtr m_world;
    std::variant<std::monostate, RigidBody2D::Ptr, RigidBody3D::Ptr> m_body;
};
