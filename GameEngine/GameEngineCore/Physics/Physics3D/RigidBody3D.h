#pragma once
#include <cstdint>
#include <memory>
#include "../Common/Math.h"

class RigidBody3D
{
public:
    using Ptr = std::shared_ptr<RigidBody3D>;

    double mass{ 1.0 };
    double invMass{ 1.0 };
    double linearDamping{ 0.0 };
    Vec3 position{ 0.0, 0.0, 0.0 };
    Vec3 velocity{ 0.0, 0.0, 0.0 };
    Vec3 force{ 0.0, 0.0, 0.0 };
    Vec3 gravity{ 0.0, -9.81, 0.0 };
    double gravityScale{ 1.0 };
    bool enableGravity{ true };
    bool enableNBodyGravity{ false };
    double radius{ 1.0 };

    void Integrate(double dt);
    void AddForce(const Vec3& f) { force += f; }
    void ClearForces() { force = Vec3{}; }
    void SetMass(double m);
};

