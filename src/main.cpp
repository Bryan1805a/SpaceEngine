#include <iostream>
#include <iomanip>
#include <Math/Vector3.hpp>
#include <Simulation/Simulation.hpp>
#include <Graphics/Renderer.hpp>

int main() {
    // Init gravity constant G = 1.0 and delta time = 0.01 second
    Simulation::System sim(1.0, 0.01);

    // Add three bodies into space
    // Body(mass, position, velocity)
    // Body 0: A large star in the middle, stand still
    sim.addBody(1000.0, Vector3::Zero, Vector3::Zero);

    // Body 1: A planet along with x axis
    sim.addBody(10.0, Vector3(100.0, 0.0, 0.0), Vector3(0.0, 3.16, 0.0));

    // Body 2: A planet along with -x axis
    sim.addBody(1.0, Vector3(-150, 0.0, 0.0), Vector3(0.0, -2.58, 0.0));

    // Init graphics window (1280x720)
    Graphics::Renderer renderer(1280, 720, "Three-Body Simulation");

    std::cout << "Running simulation" << std::endl;

    while (!renderer.shouldClose()) {
        renderer.pollEvents();
        sim.step();
        renderer.clear();

        renderer.draw(sim.getBodyCount(), sim.getPositions(), sim.getMasses());

        renderer.swapBuffers();
    }

    std::cout << "Simulation ended" << std::endl;
    return 0;
}