#pragma once
#include <vector>
#include <memory>
#include "../Common/PhysicsWorld.h"
#include "../Common/Math.h"
#include "RigidBody2D.h"

class PhysicsWorld2D final : public IPhysicsWorld
{
public:
    PhysicsWorld2D() = default;

    RigidBody2D::Ptr CreateBody();
    void RemoveAllBodies();

    void Step(double dt) override;
    void Clear() override { RemoveAllBodies(); }

    void SetGlobalGravity(const Vec2& g) { m_globalGravity = g; }
    const Vec2& GlobalGravity() const { return m_globalGravity; }

    // Enable simple N-body gravity between dynamic bodies (O(n^2) demo-friendly).
    void SetNBodyGravityEnabled(bool enabled) { m_nBodyGravity = enabled; }

    const std::vector<RigidBody2D::Ptr>& Bodies() const { return m_bodies; }

private:
    std::vector<RigidBody2D::Ptr> m_bodies;
    Vec2 m_globalGravity{ 0.0, -9.81 };
    bool m_nBodyGravity{ false };
};
