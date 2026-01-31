#include "PhysicsWorld2D.h"
#include <algorithm>

RigidBody2D::Ptr PhysicsWorld2D::CreateBody()
{
    auto body = std::make_shared<RigidBody2D>();
    body->gravity = m_globalGravity;
    m_bodies.push_back(body);
    return body;
}

void PhysicsWorld2D::RemoveAllBodies()
{
    m_bodies.clear();
}

void PhysicsWorld2D::Step(double dt)
{
    if (dt <= 0.0) return;

    // N-body gravity (naive). For planets: enable on selected bodies via enableNBodyGravity flag.
    if (m_nBodyGravity && m_bodies.size() > 1)
    {
        const double G = 6.67430e-11; // gravitational constant (scaled by caller mass units)
        for (size_t i = 0; i < m_bodies.size(); ++i)
        {
            auto& a = m_bodies[i];
            if (!a || !a->enableNBodyGravity || a->invMass == 0.0) continue;
            for (size_t j = i + 1; j < m_bodies.size(); ++j)
            {
                auto& b = m_bodies[j];
                if (!b || !b->enableNBodyGravity || b->invMass == 0.0) continue;

                Vec2 delta = b->position - a->position;
                double distSq = std::max(delta.LengthSq(), 1e-6);
                Vec2 dir = delta / std::sqrt(distSq);
                double forceMag = G * a->mass * b->mass / distSq;
                Vec2 f = dir * forceMag;
                a->AddForce(f);
                b->AddForce(f * -1.0);
            }
        }
    }

    for (auto& b : m_bodies)
    {
        if (!b) continue;
        if (b->enableGravity) b->gravity = m_globalGravity;
        b->Integrate(dt);
    }
}

