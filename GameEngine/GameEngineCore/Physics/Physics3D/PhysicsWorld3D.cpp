#include "PhysicsWorld3D.h"
#include <algorithm>

RigidBody3D::Ptr PhysicsWorld3D::CreateBody()
{
    auto body = std::make_shared<RigidBody3D>();
    body->gravity = m_globalGravity;
    m_bodies.push_back(body);
    return body;
}

void PhysicsWorld3D::RemoveAllBodies()
{
    m_bodies.clear();
}

void PhysicsWorld3D::Step(double dt)
{
    if (dt <= 0.0) return;

    if (m_nBodyGravity && m_bodies.size() > 1)
    {
        const double G = 6.67430e-11;
        for (size_t i = 0; i < m_bodies.size(); ++i)
        {
            auto& a = m_bodies[i];
            if (!a || !a->enableNBodyGravity || a->invMass == 0.0) continue;
            for (size_t j = i + 1; j < m_bodies.size(); ++j)
            {
                auto& b = m_bodies[j];
                if (!b || !b->enableNBodyGravity || b->invMass == 0.0) continue;

                Vec3 delta = b->position - a->position;
                double distSq = std::max(delta.LengthSq(), 1e-6);
                Vec3 dir = delta / std::sqrt(distSq);
                double forceMag = G * a->mass * b->mass / distSq;
                Vec3 f = dir * forceMag;
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

