#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <Math/Vector3.hpp>
#include <Simulation/Simulation.hpp>
#include <Graphics/Renderer.hpp>

int main() {
    // Initialise Engine
    // G = 1.0, dt = 0.01
    Simulation::System sim(1.0, 0.01);

    // Create a super-massive gravitational star at the center (Index 0)
    double centerMass = 10000.0;
    sim.addBody(centerMass, Vector3::Zero, Vector3::Zero);

    // Init random number generator
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> distRadius(100.0, 800.0);
    std::uniform_real_distribution<double> distAngle(0.0, 2.0 * 3.1415926535);
    std::uniform_real_distribution<double> distY(-15.0, 15.0);
    std::uniform_real_distribution<double> distMass(1.0, 5.0);

    // Create 1000 asteroids
    int numPlanets = 1000;
    for (int i = 0; i < numPlanets; ++i) {
       //  Random coordinates with disk shaped
       double r = distRadius(gen);
       double theta = distAngle(gen);

       double x = r * std::cos(theta);
       double z = r * std::sin(theta);
       double y = distY(gen);

       Vector3 pos(x, y, z);

       // Calculating tangential velocity to maintain the trajectory
       double v_mag = std::sqrt(1.0 * centerMass / r);

       // Use the mathematical cross product to obtain a vector perpendicular to the radial direction
       Vector3 v_dir(z, 0.0, -x);
       double dir_len = std::hypot(z, x);
       v_dir = v_dir / dir_len;

       Vector3 vel = v_dir * v_mag;

       // Add to the system
       sim.addBody(distMass(gen), pos, vel);
    }

    // Init graphics
    Graphics::Renderer renderer(1280, 720, "Galaxy Simulation");
    std::cout << "Initialised 1000 asteroids" << std::endl;
    std::cout << "Press X to quit" << std::endl;

    // Delta Time vars
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Main loop
    while (!renderer.shouldClose()) {
        // Update Delta Time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Operating System Event Handling and Movement Logic
        renderer.pollEvents();
        renderer.processInput(deltaTime);
        sim.step();
        renderer.clear();

        // Upload 1001 objects into GPU
        renderer.draw(sim.getBodyCount(), sim.getPositions(), sim.getMasses());

        // Draw UI
        renderer.beginUI();

        // Init Inspector
        renderer.renderUI(sim.getBodyCount());

        // UI rendering overlaid on 3D graphics
        renderer.endUI();
        
        renderer.swapBuffers();
    }

    std::cout << "Simulation has ended" << std::endl;
    return 0;
}