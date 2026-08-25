#include <Simulation/Simulation.hpp>

namespace Simulation {
    System::System(double gravityConstant, double timeStep)
        : gravity(gravityConstant), dt(timeStep) {}

    int System::addBody(const PlanetDesc& desc) {
        return world.addBody(desc);
    }

    void System::removeBody(size_t index) {
        world.removeBody(index);
    }

    void System::step() {
        // Verlet: integrate positions/orientation and the first half of velocity
        integrator.update(world, dt);

        // Compute new accelerations (gravity) for the freshly updated positions
        gravity.compute(world);

        // Verlet: complete the velocity update using the new accelerations
        integrator.finalizeVelocity(world, dt);

        // Merge colliding bodies
        collision.resolve(world);

        // Recompute temperatures from the new configuration
        thermodynamics.update(world);
    }
}
