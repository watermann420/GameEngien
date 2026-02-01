#include "EditorPhysics.h"

#include <algorithm>
#include <cmath>

std::vector<Body> g_bodies;
double g_gravity = 12000.0;
double g_sceneScale = 1.0;

void InitPhysics()
{
    g_bodies.clear();
    Body sun{};
    sun.mass = 8000.0;
    sun.radius = 10;
    sun.color = RGB(240, 200, 90);
    sun.fixed = true;
    g_bodies.push_back(sun);

    Body planet{};
    planet.mass = 4.0;
    planet.radius = 6;
    planet.color = RGB(80, 160, 240);
    planet.x = 140.0;
    planet.y = 0.0;
    double r = 140.0;
    double speed = std::sqrt(g_gravity * sun.mass / r);
    planet.vx = 0.0;
    planet.vy = speed;
    g_bodies.push_back(planet);

    Body moon{};
    moon.mass = 1.5;
    moon.radius = 4;
    moon.color = RGB(200, 200, 220);
    moon.x = 210.0;
    moon.y = 0.0;
    double r2 = 210.0;
    double speed2 = std::sqrt(g_gravity * sun.mass / r2);
    moon.vx = 0.0;
    moon.vy = speed2;
    g_bodies.push_back(moon);
}

void StepPhysics(double dt)
{
    if (g_bodies.empty()) return;
    dt = std::min(dt, 0.05);
    Body& sun = g_bodies[0];

    for (size_t i = 1; i < g_bodies.size(); ++i)
    {
        Body& b = g_bodies[i];
        double dx = b.x - sun.x;
        double dy = b.y - sun.y;
        double dist2 = dx * dx + dy * dy;
        double dist = std::sqrt(dist2) + 0.0001;
        double inv = 1.0 / (dist * dist * dist);
        double ax = -g_gravity * sun.mass * dx * inv;
        double ay = -g_gravity * sun.mass * dy * inv;
        b.vx += ax * dt;
        b.vy += ay * dt;
    }

    for (auto& b : g_bodies)
    {
        if (b.fixed) continue;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
    }
}
