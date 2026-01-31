#pragma once
#include <vector>
#include <memory>
#include "../Common/PhysicsWorld.h"
#include "../Common/Math.h"
#include "RigidBody3D.h"

class PhysicsWorld3D final : public IPhysicsWorld
{
public:
    PhysicsWorld3D() = default;

    RigidBody3D::Ptr CreateBody();
    void RemoveAllBodies();

    void Step(double dt) override;
    void Clear() override { RemoveAllBodies(); }

    void SetGlobalGravity(const Vec3& g) { m_globalGravity = g; }
    const Vec3& GlobalGravity() const { return m_globalGravity; }

    void SetNBodyGravityEnabled(bool enabled) { m_nBodyGravity = enabled; }

    const std::vector<RigidBody3D::Ptr>& Bodies() const { return m_bodies; }

private:
    std::vector<RigidBody3D::Ptr> m_bodies;
    Vec3 m_globalGravity{ 0.0, -9.81, 0.0 };
    bool m_nBodyGravity{ false };
};
