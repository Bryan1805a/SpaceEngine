#pragma once
#include <Simulation/World.hpp>
#include <Physics/Octree.hpp>

namespace Simulation {
    // System 1: Velocity and Displacement Integration (Verlet + Quaternion)
    class Integrator {
    public:
        void update(World& world, double dt);
        void finalizeVelocity(World& world, double dt);
    };

    // System 2: Gravitational Force Calculation using Octree
    class GravitySolver {
    private:
        Physics::Octree octree;
        double G;
    public:
        GravitySolver(double gravityConstant) : G(gravityConstant) {}
        void compute(World& world);
    };

    // System 3: Collision and accretion processing
    class CollisionSystem {
    public:
        void resolve(World& world);
    };

    // System 4: Thermodynamics
    class Thermodynamics {
    public:
        void update(World& world);
    };
}
