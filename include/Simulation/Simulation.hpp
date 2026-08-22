#pragma once
#include <vector>
#include <Math/Vector3.hpp>
#include <Physics/Octree.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Simulation {
    // Classification of celestial bodies
    enum class BodyType {STAR, ROCKY_PLANET, GAS_GIANT, ICE_MOON, ASTEROID};
    
    struct PlanetDesc {
        double mass;
        Vector3 position;
        Vector3 velocity;
        glm::vec3 angularVelocity = glm::vec3(0.0f);

        BodyType type = BodyType::ROCKY_PLANET;
        double albedo = 0.3; // Earth = 0.3 (Reflects 30% of light)
        double greenhouse = 1.0; // Greenhouse effect coefficient
        double temperature = 0.0; // Kelvin (Automatically calculated)
        double radius = 0.05; // Radius in AU (exaggerated for visibility)
    };

    class System {
        private:
            std::vector<double> masses;
            std::vector<Vector3> positions;
            std::vector<Vector3> velocities;
            std::vector<Vector3> accelerations;

            // Quaternion array
            std::vector<glm::quat> orientations; // Current direction of rotation
            std::vector<glm::vec3> angularVelocities; // Angular velocity (Rotation axis + Speed)

            // Geological and Temperature data arrays
            std::vector<BodyType> types;
            std::vector<double> albedos;
            std::vector<double> greenhouses;
            std::vector<double> temperatures;
            std::vector<double> radii;

            Physics::Octree octree;

            double G; // Gravity const
            double dt; // Delta time

            void computeAcceleration();
            void handleCollisions(); // Fused collision handling algorithm
            void computeThermodynamics(); // Temperature calculation function

        public:
            System(double gravityConstant, double timeStep);

            // Add a body to simulation
            void addBody(const PlanetDesc& desc);
            // Remove a body
            void removeBody(size_t index);
            // Execute a delta time jump t (Verlet Algorithm)
            void step();

            
            // Getter functions that allow graphics components to read the data
            size_t getBodyCount() const { return masses.size(); }
            const std::vector<Vector3>& getPositions() const { return positions; }
            const std::vector<double>& getMasses() const { return masses; }
            const std::vector<glm::quat>& getOrientations() const { return orientations; }

            // Provide data for the shader to use for coloring later
            const std::vector<double>& getTemperatures() const { return temperatures; }
            const std::vector<BodyType>& getTypes() const { return types; }
            const std::vector<double>& getRadii() const { return radii; }

            // Change delta time function
            void setDt(double newDt) {dt = newDt;}            
    };
}