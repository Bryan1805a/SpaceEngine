#pragma once
#include <vector>
#include <Math/Vector3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Simulation {
    enum class BodyType { STAR, ROCKY_PLANET, GAS_GIANT, ICE_MOON, ASTEROID };

    struct PlanetDesc {
        double mass;
        Vector3 position;
        Vector3 velocity;
        glm::vec3 angularVelocity = glm::vec3(0.0f);
        BodyType type = BodyType::ROCKY_PLANET;
        double albedo = 0.3;
        double greenhouse = 1.0;
        double temperature = 0.0;
        double radius = 0.05;
    };

    class World {
    public:
        // Set to public to allow systems to easily perform high-speed read/write operations
        std::vector<double> masses;
        std::vector<Vector3> positions;
        std::vector<Vector3> velocities;
        std::vector<Vector3> accelerations;

        std::vector<glm::quat> orientations;
        std::vector<glm::vec3> angularVelocities;

        std::vector<BodyType> types;
        std::vector<double> albedos;
        std::vector<double> greenhouses;
        std::vector<double> temperatures;
        std::vector<double> radii;

        // Basic lifecycle management functions
        void addBody(const PlanetDesc& desc);
        void removeBody(size_t index);
        size_t getBodyCount() const { return masses.size(); }
        void resetAccelerations();
    };
}
