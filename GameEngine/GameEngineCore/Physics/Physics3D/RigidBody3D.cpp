#include "RigidBody3D.h"

void RigidBody3D::SetMass(double m)
{
    mass = (m <= 0.0) ? 0.0 : m;
    invMass = (mass > 0.0) ? 1.0 / mass : 0.0;
}

void RigidBody3D::Integrate(double dt)
{
    if (invMass == 0.0 || dt <= 0.0) return;

    Vec3 acc = force * invMass;
    if (enableGravity)
        acc += gravity * gravityScale;

    velocity += acc * dt;
    velocity *= (1.0 / (1.0 + linearDamping * dt));
    position += velocity * dt;

    ClearForces();
}

