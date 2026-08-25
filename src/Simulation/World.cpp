#include <Simulation/World.hpp>

namespace Simulation {
    int World::addBody(const PlanetDesc& desc) {
        int id = nextId++;
        std::string entityName = desc.name.empty() ? ("Object #" + std::to_string(names.size())) : desc.name;
        ids.push_back(id);
        names.push_back(entityName);
        masses.push_back(desc.mass);
        positions.push_back(desc.position);
        velocities.push_back(desc.velocity);
        accelerations.push_back(Vector3::Zero); // The initial acceleration is always zero

        // Identity Quaternion
        orientations.push_back(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        angularVelocities.push_back(desc.angularVelocity);

        types.push_back(desc.type);
        albedos.push_back(desc.albedo);
        greenhouses.push_back(desc.greenhouse);
        temperatures.push_back(desc.temperature);
        radii.push_back(desc.radius);

        assetIndices.push_back(desc.assetIndex);
        parentIds.push_back(desc.parentId);
        return id;
    }

    void World::removeBody(size_t index) {
        if (index >= masses.size()) return;

        // Swap the last element with the element to be deleted
        size_t lastIdx = masses.size() - 1;
        ids[index] = ids[lastIdx];
        names[index] = names[lastIdx];
        masses[index] = masses[lastIdx];
        positions[index] = positions[lastIdx];
        velocities[index] = velocities[lastIdx];
        accelerations[index] = accelerations[lastIdx];
        orientations[index] = orientations[lastIdx];
        angularVelocities[index] = angularVelocities[lastIdx];
        types[index] = types[lastIdx];
        albedos[index] = albedos[lastIdx];
        greenhouses[index] = greenhouses[lastIdx];
        temperatures[index] = temperatures[lastIdx];
        radii[index] = radii[lastIdx];
        assetIndices[index] = assetIndices[lastIdx];
        parentIds[index] = parentIds[lastIdx];

        // Pop last index
        ids.pop_back();
        names.pop_back();
        masses.pop_back();
        positions.pop_back();
        velocities.pop_back();
        accelerations.pop_back();
        orientations.pop_back();
        angularVelocities.pop_back();
        types.pop_back();
        albedos.pop_back();
        greenhouses.pop_back();
        temperatures.pop_back();
        radii.pop_back();
        assetIndices.pop_back();
        parentIds.pop_back();
    }

    void World::resetAccelerations() {
        for (size_t i = 0; i < accelerations.size(); ++i) {
            accelerations[i] = Vector3::Zero;
        }
    }
}
