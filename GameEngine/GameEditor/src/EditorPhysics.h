#pragma once

#include <vector>
#define NOMINMAX
#include <windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

struct Body
{
    double x{};
    double y{};
    double vx{};
    double vy{};
    double mass{};
    int radius{};
    COLORREF color{};
    bool fixed{};
};

extern std::vector<Body> g_bodies;
extern double g_gravity;
extern double g_sceneScale;

void InitPhysics();
void StepPhysics(double dt);
