#include <Simulation/Simulation.hpp>
#include <cmath>
#include <execution>
#include <numeric>

namespace Simulation {
    System::System(double gravityConstant, double timeStep)
        : G(gravityConstant), dt(timeStep), octree(positions, masses) {}

    void System::addBody(const PlanetDesc& desc) {
        masses.push_back(desc.mass);
        positions.push_back(desc.position);
        velocities.push_back(desc.velocity);
        accelerations.push_back(Vector3::Zero); // The initial acceleration is always zero

        // Identity Quaternion
        orientations.push_back(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        angularVelocities.push_back(desc.angularVelocity);

        types.push_back(desc.type);
        albedos.push_back(desc.albedo);
        greenhouses.push_back(desc.greenhouse);
        temperatures.push_back(desc.temperature);
        radii.push_back(desc.radius);
    }

    void System::removeBody(size_t index) {
        if (index >= masses.size()) return;

        // Swap the last element with the element to be deleted
        size_t lastIdx = masses.size() - 1;
        masses[index] = masses[lastIdx];
        positions[index] = positions[lastIdx];
        velocities[index] = velocities[lastIdx];
        accelerations[index] = accelerations[lastIdx];
        orientations[index] = orientations[lastIdx];
        angularVelocities[index] = angularVelocities[lastIdx];
        types[index] = types[lastIdx];
        albedos[index] = albedos[lastIdx];
        greenhouses[index] = greenhouses[lastIdx];
        temperatures[index] = temperatures[lastIdx];
        radii[index] = radii[lastIdx];

        // Pop last index
        masses.pop_back();
        positions.pop_back();
        velocities.pop_back();
        accelerations.pop_back();
        orientations.pop_back();
        angularVelocities.pop_back();
        types.pop_back();
        albedos.pop_back();
        greenhouses.pop_back();
        temperatures.pop_back();
        radii.pop_back();
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

    void System::computeThermodynamics() {
        size_t n = masses.size();

        // Find the position of the host star (the Sun)
        // Use the first star as the primary heat source
        // FUTURE UPDATE: In a multi-star system, the total heat sources from multiple stars are combined
        int starIndex = -1;
        for (size_t i = 0; i < n; ++i) {
            if (types[i] == BodyType::STAR) {
                starIndex = i;
                break;
            }
        }

        for (size_t i = 0; i < n; ++i) {
            if (types[i] == BodyType::STAR) {
                temperatures[i] = 5778.0; // Sun's surface temperature
                continue;
            }

            if (starIndex != -1) {
                // Calculate the distance to the Sun (in AU)
                Vector3 diff = positions[i] - positions[starIndex];
                double distAU = diff.length();

                if (distAU > 0.001) {
                    // Stefan-Boltzmann heat balance equation
                    double t = 278.3 * std::pow(1.0 - albedos[i], 0.25) / std::sqrt(distAU);
                    temperatures[i] = t * greenhouses[i];
                }
            }
            else {
                // A rogue planet without a host star will freeze
                temperatures[i] = 2.7; // Cosmic Microwave Background Radiation (CMBR) temperature
            }
        }
    }

    void System::handleCollisions() {
        for (size_t i = 0; i < masses.size(); ++i) {
            for (size_t j = i + 1; j < masses.size(); ) {
                // Calculate Collision Radius (explicit physical radius in AU)
                double radiusI = radii[i];
                double radiusJ = radii[j];
                double collisionDist = radiusI + radiusJ;

                // Calculate the squared distance
                Vector3 diff = positions[i] - positions[j];
                double distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

                // If collision
                if (distSq < collisionDist * collisionDist) {
                    double newMass = masses[i] + masses[j];

                    // Apply the law of conservation of momentum
                    velocities[i] = (velocities[i] * masses[i] + velocities[j] * masses[j]) / newMass;
                    angularVelocities[i] = angularVelocities[i] + angularVelocities[j];

                    // The new position is the center of the two objects
                    positions[i] = (positions[i] * masses[i] + positions[j] * masses[j]) / newMass;
                    masses[i] = newMass;

                    // Delete the collison planet
                    // Swap and Pop - O(1) 
                    size_t lastIdx = masses.size() - 1;

                    masses[j] = masses[lastIdx];
                    positions[j] = positions[lastIdx];
                    velocities[j] = velocities[lastIdx];
                    accelerations[j] = accelerations[lastIdx];
                    orientations[j] = orientations[lastIdx];
                    angularVelocities[j] = angularVelocities[lastIdx];
                    types[j] = types[lastIdx];
                    albedos[j] = albedos[lastIdx];
                    greenhouses[j] = greenhouses[lastIdx];
                    temperatures[j] = temperatures[lastIdx];
                    radii[j] = radii[lastIdx];

                    // Delete last index
                    masses.pop_back();
                    positions.pop_back();
                    velocities.pop_back();
                    accelerations.pop_back();
                    orientations.pop_back();
                    angularVelocities.pop_back();
                    types.pop_back();
                    albedos.pop_back();
                    greenhouses.pop_back();
                    temperatures.pop_back();
                    radii.pop_back();
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

        // Update orientation based on angular velocity
        for (size_t i = 0; i < n; ++i) {
            float speed = glm::length(angularVelocities[i]); // Rotate speed (Radian/s)
            if (speed > 0.0001f) {
                // Create a quaternion representing the rotation over a single frame (dt)
                glm::vec3 axis = angularVelocities[i] / speed;
                glm::quat spin = glm::angleAxis(speed * (float)dt, axis);
                orientations[i] = glm::normalize(spin * orientations[i]);
            }
        }

        // Calculate new acceleration based on the newly updated new position
        computeAcceleration();

        // Update half (1/2) of the remaining velocity using the new acceleration
        for (size_t i = 0; i < n; ++i) {
            // v_new = v_half + 0.5*a_new*dt
            velocities[i] += accelerations[i] * (0.5 * dt);
        }

        handleCollisions();

        // Tracking temperature changes as the planet's orbit shifts
        computeThermodynamics();
    }
}