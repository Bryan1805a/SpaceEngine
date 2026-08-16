#pragma once
#include <vector>
#include <Physics/Body.hpp>

namespace Simulation {
    class System {
        private:
            std::vector<Physics::Body> bodies;
            double G; // Gravity const
            double dt; // Delta time

            void computeAcceleration();

        public:
            System(double gravityConstant, double timeStep);

            // Add a body to simulation
            void addBody(const Physics::Body& body);

            // Execute a delta time jump t (Verlet Algorithm)
            void step();

            // Retrieve the list of objects
            const std::vector<Physics::Body>& getBodies() const;
    };
}