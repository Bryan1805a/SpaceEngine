#pragma once
#include <vector>
#include <Math/Vector3.hpp>
#include <Physics/Octree.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Simulation {
    class System {
        private:
            std::vector<double> masses;
            std::vector<Vector3> positions;
            std::vector<Vector3> velocities;
            std::vector<Vector3> accelerations;

            // Quaternion array
            std::vector<glm::quat> orientations; // Current direction of rotation
            std::vector<glm::vec3> angularVelocities; // Angular velocity (Rotation axis + Speed)

            Physics::Octree octree;

            double G; // Gravity const
            double dt; // Delta time

            void computeAcceleration();
            void handleCollisions(); // Fused collision handling algorithm

        public:
            System(double gravityConstant, double timeStep);

            // Add a body to simulation
            void addBody(double mass, const Vector3& pos, const Vector3& vel, const glm::vec3& angularVel = glm::vec3(0.0f));
            const std::vector<glm::quat>& getOrientations() const { return orientations; }

            // Getter functions that allow graphics components to read the data
            size_t getBodyCount() const { return masses.size(); }
            const std::vector<Vector3>& getPositions() const { return positions; }
            const std::vector<double>& getMasses() const { return masses; }

            // Execute a delta time jump t (Verlet Algorithm)
            void step();

            // Change delta time function
            void SetDt(double newDt) {dt = newDt;}

            // Remove a body
            void removeBody(size_t index);
    };
}