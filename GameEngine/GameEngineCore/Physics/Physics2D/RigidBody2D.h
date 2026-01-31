#pragma once
#include <cstdint>
#include <memory>
#include "../Common/Math.h"

class RigidBody2D
{
public:
    using Ptr = std::shared_ptr<RigidBody2D>;

    double mass{ 1.0 };
    double invMass{ 1.0 };
    double linearDamping{ 0.0 }; // 0 = none
    Vec2 position{ 0.0, 0.0 };
    Vec2 velocity{ 0.0, 0.0 };
    Vec2 force{ 0.0, 0.0 };
    Vec2 gravity{ 0.0, -9.81 };
    double gravityScale{ 1.0 };
    bool enableGravity{ true };
    bool enableNBodyGravity{ false };
    double radius{ 1.0 }; // for simple overlap checks / visualization

    void Integrate(double dt);

    void AddForce(const Vec2& f) { force += f; }
    void ClearForces() { force = Vec2{}; }

    void SetMass(double m);
};

