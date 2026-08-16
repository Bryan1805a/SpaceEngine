#include <Simulation/Simulation.hpp>
#include <cmath>

namespace Simulation {
    System::System(double gravityConstant, double timeStep)
        : G(gravityConstant), dt(timeStep) {}

    void System::addBody(const Physics::Body& body) {
        bodies.push_back(body);
    }

    const std::vector<Physics::Body>& System::getBodies() const {
        return bodies;
    }

    void System::computeAcceleration() {
        // Reset acceleration to 0
        for (auto& body : bodies) {
            body.acceleration = Vector3::Zero;
        }

        // Loop
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = 0; j < bodies.size(); ++j) {
                Physics::Body& bodyI = bodies[i];
                Physics::Body& bodyJ = bodies[j];

                // Calculate distance vector from i to j
                Vector3 r_ij = bodyJ.position - bodyI.position;

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
                double accel_term_I = (G * bodyJ.mass) / r_cubed;
                double accel_term_J = (G * bodyI.mass) / r_cubed;

                // Update acceleration for both objects (Newton's Third Law)
                bodyI.acceleration += r_ij * accel_term_I;
                bodyJ.acceleration -= r_ij * accel_term_J;
            }
        }
    }

    void System::step() {
        // Update Position using Velocity and Acceleration (OLD)
        // Also update half (1/2) of the velocity beforehand
        for (auto& body : bodies) {
            // r_new = r_old + v*dt + 0.5*a*dt^2
            body.position += body.velocity * dt + body.acceleration * (0.5 * dt * dt);

            // v_half = v_old + 0.5 * a_old * dt
            body.velocity += body.acceleration * (0.5 * dt);
        }

        // Calculate new acceleration based on the newly updated new position
        computeAcceleration();

        // Update half (1/2) of the remaining velocity using the new acceleration
        for (auto& body : bodies) {
            // v_new = v_half + 0.5*a_new*dt
            body.velocity += body.acceleration * (0.5 * dt);
        }
    }
}