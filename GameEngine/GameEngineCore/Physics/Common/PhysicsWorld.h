#pragma once
#include <memory>
#include <vector>

// Minimal dimension-agnostic physics world interface.
class IPhysicsWorld
{
public:
    virtual ~IPhysicsWorld() = default;

    virtual void Step(double dt) = 0;
    virtual void Clear() = 0;
};

using PhysicsWorldPtr = std::shared_ptr<IPhysicsWorld>;

