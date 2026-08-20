#pragma once
#include <vector>
#include <Math/Vector3.hpp>
#include <Physics/Octree.hpp>

namespace Simulation {
    class System {
        private:
            std::vector<double> masses;
            std::vector<Vector3> positions;
            std::vector<Vector3> velocities;
            std::vector<Vector3> accelerations;

            Physics::Octree octree;

            double G; // Gravity const
            double dt; // Delta time

            void computeAcceleration();
            void handleCollisions(); // Fused collision handling algorithm

        public:
            System(double gravityConstant, double timeStep);

            // Add a body to simulation
            void addBody(double mass, const Vector3& pos, const Vector3& vel);

            // Getter functions that allow graphics components to read the data
            size_t getBodyCount() const { return masses.size(); }
            const std::vector<Vector3>& getPositions() const { return positions; }
            const std::vector<double>& getMasses() const { return masses; }

            // Execute a delta time jump t (Verlet Algorithm)
            void step();
    };
}