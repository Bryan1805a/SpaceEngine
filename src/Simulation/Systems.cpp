#include <Simulation/Systems.hpp>
#include <cmath>
#include <execution>
#include <numeric>
#include <glm/gtc/quaternion.hpp>

namespace Simulation {
    void Integrator::update(World& world, double dt) {
        size_t n = world.masses.size();

        // Update Position using Velocity and Acceleration (OLD)
        // Also update half (1/2) of the velocity beforehand
        for (size_t i = 0; i < n; ++i) {
            // r_new = r_old + v*dt + 0.5*a*dt^2
            world.positions[i] += world.velocities[i] * dt + world.accelerations[i] * (0.5 * dt * dt);

            // v_half = v_old + 0.5 * a_old * dt
            world.velocities[i] += world.accelerations[i] * (0.5 * dt);
        }

        // Update orientation based on angular velocity
        for (size_t i = 0; i < n; ++i) {
            float speed = glm::length(world.angularVelocities[i]); // Rotate speed (Radian/s)
            if (speed > 0.0001f) {
                // Create a quaternion representing the rotation over a single frame (dt)
                glm::vec3 axis = world.angularVelocities[i] / speed;
                glm::quat spin = glm::angleAxis(speed * (float)dt, axis);
                world.orientations[i] = glm::normalize(spin * world.orientations[i]);
            }
        }
    }

    void Integrator::finalizeVelocity(World& world, double dt) {
        size_t n = world.masses.size();

        // Update half (1/2) of the remaining velocity using the new acceleration
        for (size_t i = 0; i < n; ++i) {
            // v_new = v_half + 0.5*a_new*dt
            world.velocities[i] += world.accelerations[i] * (0.5 * dt);
        }
    }

    void GravitySolver::compute(World& world) {
        size_t n = world.masses.size();
        if (n == 0) return;

        // Reset acceleration array to 0
        world.resetAccelerations();

        // Connect the tree to the current frame's data and rebuild it from scratch
        octree.bind(world.positions, world.masses);
        octree.build();

        // Multithreading
        // Calculate acceleration for each planet by query tree
        double theta = 0.5; // Just choose it for no reason

        std::vector<size_t> indices(n);
        std::iota(indices.begin(), indices.end(), 0);

        // Distribute task to the CPU
        std::for_each(std::execution::par, indices.begin(), indices.end(), [&](size_t i) {
            // Each CPU core will automatically take an index i and perform calculations independently
            world.accelerations[i] = octree.calculateAcceleration(i, theta, G);
        });
    }

    void CollisionSystem::resolve(World& world) {
        for (size_t i = 0; i < world.masses.size(); ++i) {
            for (size_t j = i + 1; j < world.masses.size(); ) {
                // In our model, planets are visually scaled up immensely for visibility.
                // This causes them to "collide" physically even when they are millions of km apart.
                // To prevent moons and planets from merging, we restrict collisions to ASTEROIDS.
                if (world.types[i] != BodyType::ASTEROID && world.types[j] != BodyType::ASTEROID) {
                    ++j;
                    continue;
                }

                // Calculate Collision Radius (explicit physical radius in AU)
                double radiusI = world.radii[i];
                double radiusJ = world.radii[j];
                double collisionDist = radiusI + radiusJ;

                // Calculate the squared distance
                Vector3 diff = world.positions[i] - world.positions[j];
                double distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

                // If collision
                if (distSq < collisionDist * collisionDist) {
                    double newMass = world.masses[i] + world.masses[j];

                    // Apply the law of conservation of momentum
                    world.velocities[i] = (world.velocities[i] * world.masses[i] + world.velocities[j] * world.masses[j]) / newMass;
                    world.angularVelocities[i] = world.angularVelocities[i] + world.angularVelocities[j];

                    // The new position is the center of the two objects
                    world.positions[i] = (world.positions[i] * world.masses[i] + world.positions[j] * world.masses[j]) / newMass;
                    world.masses[i] = newMass;

                    // Delete the collison planet
                    // Swap and Pop - O(1)
                    size_t lastIdx = world.masses.size() - 1;

                    world.names[j] = world.names[lastIdx];
                    world.masses[j] = world.masses[lastIdx];
                    world.positions[j] = world.positions[lastIdx];
                    world.velocities[j] = world.velocities[lastIdx];
                    world.accelerations[j] = world.accelerations[lastIdx];
                    world.orientations[j] = world.orientations[lastIdx];
                    world.angularVelocities[j] = world.angularVelocities[lastIdx];
                    world.types[j] = world.types[lastIdx];
                    world.albedos[j] = world.albedos[lastIdx];
                    world.greenhouses[j] = world.greenhouses[lastIdx];
                    world.temperatures[j] = world.temperatures[lastIdx];
                    world.radii[j] = world.radii[lastIdx];

                    // Delete last index
                    world.names.pop_back();
                    world.masses.pop_back();
                    world.positions.pop_back();
                    world.velocities.pop_back();
                    world.accelerations.pop_back();
                    world.orientations.pop_back();
                    world.angularVelocities.pop_back();
                    world.types.pop_back();
                    world.albedos.pop_back();
                    world.greenhouses.pop_back();
                    world.temperatures.pop_back();
                    world.radii.pop_back();
                }
                else {
                    ++j;
                }
            }
        }
    }

    void Thermodynamics::update(World& world) {
        size_t n = world.masses.size();

        // Find the position of the host star (the Sun)
        // Use the first star as the primary heat source
        // FUTURE UPDATE: In a multi-star system, the total heat sources from multiple stars are combined
        int starIndex = -1;
        for (size_t i = 0; i < n; ++i) {
            if (world.types[i] == BodyType::STAR) {
                starIndex = i;
                break;
            }
        }

        for (size_t i = 0; i < n; ++i) {
            if (world.types[i] == BodyType::STAR) {
                world.temperatures[i] = 5778.0; // Sun's surface temperature
                continue;
            }

            if (starIndex != -1) {
                // Calculate the distance to the Sun (in AU)
                Vector3 diff = world.positions[i] - world.positions[starIndex];
                double distAU = diff.length();

                if (distAU > 0.001) {
                    // Stefan-Boltzmann heat balance equation
                    double t = 278.3 * std::pow(1.0 - world.albedos[i], 0.25) / std::sqrt(distAU);
                    world.temperatures[i] = t * world.greenhouses[i];
                }
            }
            else {
                // A rogue planet without a host star will freeze
                world.temperatures[i] = 2.7; // Cosmic Microwave Background Radiation (CMBR) temperature
            }
        }
    }
}
