#include <Simulation/Simulation.hpp>
#include <cmath>

namespace Simulation {
    System::System(double gravityConstant, double timeStep)
        : G(gravityConstant), dt(timeStep), octree(4000.0, positions, masses) {}

    void System::addBody(double mass, const Vector3& pos, const Vector3& vel) {
        masses.push_back(mass);
        positions.push_back(pos);
        velocities.push_back(vel);
        accelerations.push_back(Vector3::Zero); // The initial acceleration is always zero
    }

    void System::computeAcceleration() {
        size_t n = masses.size();
        if (n == 0) return;

        // Reset acceleration array to 0
        for (size_t i = 0; i < n; ++i) {
            accelerations[i] = Vector3::Zero;
        }

        // Rebuild spacial tree from scratch for current frame
        octree.build();

        // Calculate acceleration for each planet by query tree
        double theta = 0.5; // Just choose it for no reason
        for (size_t i = 0; i < n; ++i) {
            accelerations[i] = octree.calculateAcceleration(i, theta, G);
        }
    }

    void System::step() {
        size_t n = masses.size();
        // Update Position using Velocity and Acceleration (OLD)
        // Also update half (1/2) of the velocity beforehand
        for (size_t i = 0; i < n; ++i) {
            // r_new = r_old + v*dt + 0.5*a*dt^2
            positions[i] += velocities[i] * dt + accelerations[i] * (0.5 * dt * dt);

            // v_half = v_old + 0.5 * a_old * dt
            velocities[i] += accelerations[i] * (0.5 * dt);
        }

        // Calculate new acceleration based on the newly updated new position
        computeAcceleration();

        // Update half (1/2) of the remaining velocity using the new acceleration
        for (size_t i = 0; i < n; ++i) {
            // v_new = v_half + 0.5*a_new*dt
            velocities[i] += accelerations[i] * (0.5 * dt);
        }
    }
}