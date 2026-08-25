#pragma once
#include <vector>
#include <string>
#include <Math/Vector3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Simulation {
    enum class BodyType { STAR, ROCKY_PLANET, GAS_GIANT, ICE_MOON, ASTEROID };

    struct PlanetDesc {
        std::string name = "";
        double mass;
        Vector3 position;
        Vector3 velocity;
        glm::vec3 angularVelocity = glm::vec3(0.0f);
        BodyType type = BodyType::ROCKY_PLANET;
        double albedo = 0.3;
        double greenhouse = 1.0;
        double temperature = 0.0;
        double radius = 0.05;

        // Stable identity & render hints. These must travel with the body through
        // add/remove/merge (swap-and-pop) so the renderer never relies on the
        // transient array index.
        int assetIndex = -1; // Index into Renderer::planetAssets (-1 = procedural sphere)
        int parentId = -1;   // Stable id of the orbit-line parent (-1 = orbit the host star)
    };

    class World {
    public:
        // Set to public to allow systems to easily perform high-speed read/write operations
        std::vector<std::string> names;
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

        // Stable per-body identity and render hints (survive swap-and-pop).
        std::vector<int> ids;          // Unique, monotonically increasing identity
        std::vector<int> assetIndices; // Renderer model index (-1 = procedural sphere)
        std::vector<int> parentIds;    // Stable id of the orbit-line parent (-1 = star)

        // Basic lifecycle management functions
        int addBody(const PlanetDesc& desc); // Returns the assigned stable id
        void removeBody(size_t index);
        size_t getBodyCount() const { return masses.size(); }
        void resetAccelerations();

    private:
        int nextId = 0;
    };
}
