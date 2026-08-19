#include <Simulation/Simulation.hpp>
#include <cmath>

namespace Simulation {
    System::System(double gravityConstant, double timeStep)
        : G(gravityConstant), dt(timeStep) {}

    void System::addBody(double mass, const Vector3& pos, const Vector3& vel) {
        masses.push_back(mass);
        positions.push_back(pos);
        velocities.push_back(vel);
        accelerations.push_back(Vector3::Zero); // The initial acceleration is always zero
    }

    void System::computeAcceleration() {
        size_t n = masses.size();

        // Reset acceleration array to 0
        for (size_t i = 0; i < n; ++i) {
            accelerations[i] = Vector3::Zero;
        }

        // Calculate force and acceleration
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                Vector3 r_ij = positions[j] - positions[i];
                // Calculate the distance r (vector length)
                double r = r_ij.length();

                // Zero range handle
                if (r == 0) {
                    continue;
                }

                // Calculate the magnitude of the gravitational force divided by the mass
                // Based on the formula: F = G * m1 * m2 / r^3 * r_vec
                // Acceleration a1 = F / m1 = G * m2 / r^3 * r_vec
                double r_cubed = r * r * r;
                double accel_term_I = (G * masses[j]) / r_cubed;
                double accel_term_J = (G * masses[i]) / r_cubed;

                // Update acceleration for both objects (Newton's Third Law)
                accelerations[i] += r_ij * accel_term_I;
                accelerations[j] -= r_ij * accel_term_J;
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
    }
}