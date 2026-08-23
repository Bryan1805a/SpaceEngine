#pragma once
#include <vector>
#include <Math/Vector3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Simulation/World.hpp>
#include <Simulation/Systems.hpp>

namespace Simulation {
    class System {
        private:
            // Simulation state (all the body data)
            World world;

            // Sub-systems that operate on the world
            Integrator integrator;
            GravitySolver gravity;
            CollisionSystem collision;
            Thermodynamics thermodynamics;

            double dt; // Delta time

        public:
            System(double gravityConstant, double timeStep);

            // Add a body to simulation
            void addBody(const PlanetDesc& desc);
            // Remove a body
            void removeBody(size_t index);
            // Execute a delta time jump t (Verlet Algorithm)
            void step();

            // Getter functions that allow graphics components to read the data
            size_t getBodyCount() const { return world.getBodyCount(); }
            const std::vector<std::string>& getNames() const { return world.names; }
            const std::vector<Vector3>& getPositions() const { return world.positions; }
            const std::vector<Vector3>& getVelocities() const { return world.velocities; }
            const std::vector<double>& getMasses() const { return world.masses; }
            const std::vector<glm::quat>& getOrientations() const { return world.orientations; }

            // Provide data for the shader to use for coloring later
            const std::vector<double>& getTemperatures() const { return world.temperatures; }
            const std::vector<BodyType>& getTypes() const { return world.types; }
            const std::vector<double>& getRadii() const { return world.radii; }

            // Change delta time function
            void setDt(double newDt) { dt = newDt; }
    };
}
