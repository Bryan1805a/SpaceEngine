#include <iostream>
#include <iomanip>
#include <Math/Vector3.hpp>
#include <Physics/Body.hpp>
#include <Simulation/Simulation.hpp>

int main() {
    // Init gravity constant G = 1.0 and delta time = 0.01 second
    Simulation::System sim(1.0, 0.01);

    // Add three bodies into space
    // Body(mass, position, velocity)
    // Body 0: A large star in the middle, stand still
    sim.addBody(Physics::Body(1000.0, Vector3::Zero, Vector3::Zero));

    // Body 1: A planet along with x axis
    sim.addBody(Physics::Body(10.0, Vector3(100.0, 0.0, 0.0), Vector3(0.0, 3.16, 0.0)));

    // Body 2: A planet along with -x axis
    sim.addBody(Physics::Body(1.0, Vector3(-150, 0.0, 0.0), Vector3(0.0, -2.58, 0.0)));

    std::cout << "START SIMULATION" << std::endl << std::endl;

    // Config console to ouput 2 ditgits number
    std::cout << std::fixed << std::setprecision(2);

    // Main loop
    int total_steps = 1000;
    int print_interval = 200;

    for (int step = 0; step <= total_steps; ++step) {
        // Print status
        if (step % print_interval == 0) {
            std::cout << "[ Step " << step << " | Time t = " << step * 0.01 << " ]\n";

            const auto& bodies = sim.getBodies();
            for (size_t i = 0; i < bodies.size(); ++i) {
                std::cout << " Body " << i
                          << " | Position: " << bodies[i].position
                          << " | Velocity: " << bodies[i].velocity
                          << "\n";
            }
            std::cout << "---------------------------------\n";
        }

        sim.step();
    }

    return 0;
}