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
    }
}