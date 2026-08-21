#include <Simulation/Simulation.hpp>
#include <cmath>
#include <execution>
#include <numeric>

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
        
        // Multithreading
        // Calculate acceleration for each planet by query tree
        double theta = 0.5; // Just choose it for no reason
        
        std::vector<size_t> indices(n);
        std::iota(indices.begin(), indices.end(), 0);

        // Distribute task to the CPU
        std::for_each(std::execution::par, indices.begin(), indices.end(), [&](size_t i) {
            // Each CPU core will automatically take an index i and perform calculations independently
            accelerations[i] = octree.calculateAcceleration(i, theta, G);
        });
    }

    void System::handleCollisions() {
        for (size_t i = 0; i < masses.size(); ++i) {
            for (size_t j = i + 1; j < masses.size(); ) {
                // Calculate Collision Radius
                // Based on Volume Proportional to Mass
                // Spherical structure: V = 4/3 * π * R³ 
                // => R is proportional to the cube root of M
                double radiusI = std::cbrt(masses[i]);
                double radiusJ = std::cbrt(masses[j]);
                double collisionDist = radiusI + radiusJ;

                // Calculate the squared distance
                Vector3 diff = positions[i] - positions[j];
                double distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

                // If collision
                if (distSq < collisionDist * collisionDist) {
                    double newMass = masses[i] + masses[j];

                    // Apply the law of conservation of momentum
                    velocities[i] = (velocities[i] * masses[i] + velocities[j] * masses[j]) / newMass;

                    // The new position is the center of the two objects
                    positions[i] = (positions[i] * masses[i] + positions[j] * masses[j]) / newMass;
                    masses[i] = newMass;

                    // Swap and Pop - O(1)
                    size_t lastIdx = masses.size() - 1;

                    masses[j] = masses[lastIdx];
                    positions[j] = positions[lastIdx];
                    velocities[j] = velocities[lastIdx];
                    accelerations[j] = accelerations[lastIdx];

                    // Delete last index
                    masses.pop_back();
                    positions.pop_back();
                    velocities.pop_back();
                    accelerations.pop_back();
                }
                else {
                    ++j;
                }
            }
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

        handleCollisions();
    }

    void System::removeBody(size_t index) {
        if (index >= masses.size()) return;

        // Swap the last element with the element to be deleted
        size_t lastIdx = masses.size() - 1;
        masses[index] = masses[lastIdx];
        positions[index] = positions[lastIdx];
        velocities[index] = velocities[lastIdx];
        accelerations[index] = accelerations[lastIdx];

        // Pop last index
        masses.pop_back();
        positions.pop_back();
        velocities.pop_back();
        accelerations.pop_back();
    }
}