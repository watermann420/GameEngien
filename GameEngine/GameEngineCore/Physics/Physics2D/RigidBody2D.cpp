#include "RigidBody2D.h"

void RigidBody2D::SetMass(double m)
{
    mass = (m <= 0.0) ? 0.0 : m;
    invMass = (mass > 0.0) ? 1.0 / mass : 0.0;
}

void RigidBody2D::Integrate(double dt)
{
    if (invMass == 0.0 || dt <= 0.0) return; // static / immovable

    // Semi-implicit Euler
    Vec2 acc = force * invMass;
    if (enableGravity)
        acc += gravity * gravityScale;

    velocity += acc * dt;

    // Damping
    velocity *= (1.0 / (1.0 + linearDamping * dt));

    position += velocity * dt;

    ClearForces();
}

