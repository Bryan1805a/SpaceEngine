#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <imgui.h>
#include <Math/Vector3.hpp>
#include <Simulation/Simulation.hpp>
#include <Graphics/Renderer.hpp>

int raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const std::vector<Vector3>& positions, const std::vector<double>& masses) {
    int hitIndex = -1;
    float minDistance = 999999.0f; // Find the nearest planet if the ray passes through multiple planets

    for (size_t i = 0; i < positions.size(); ++i) {
        glm::vec3 center((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
        float radius = (float)std::cbrt(masses[i]);

        // Calculate the discriminant (Delta) for the quadratic equation
        glm::vec3 oc = rayOrigin - center;
        float b = glm::dot(oc, rayDir);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - c;

        // If Delta > 0 -> Penetrating ray (2 solutions)
        // Then select the smaller solution (the surface closest to the camera)
        if (discriminant > 0) {
            float t = -b - sqrt(discriminant);
            if (t > 0 && t < minDistance) {
                minDistance = t;
                hitIndex = i;
            }
        }
    }
    return hitIndex;
}

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

    // UI status vars
    float timeScale = 1.0f;         // Time speed
    float newPlaneMass = 5000.0f;   // Mass of the planet about to be launched
    float shootSpeed = 300.0f;      // Lauch velocity
    const double baseDt = 0.01f;    // Base time step

    // Main loop
    int selectedEntity = -1;
    while (!renderer.shouldClose()) {
        // Update Delta Time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Operating System Event Handling and Movement Logic
        renderer.pollEvents();
        renderer.processInput(deltaTime);

        // Apply time scale for physics
        sim.SetDt(baseDt * timeScale);
        sim.step();
        renderer.clear();

        // Upload 1001 objects into GPU
        renderer.draw(sim.getBodyCount(), sim.getPositions(), sim.getMasses());

        // Draw UI
        renderer.beginUI();

        ImGui::Begin("Space Engine Control Panel");

        ImGui::Text("Performance: %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::Text("Active Entities: %zu", sim.getBodyCount());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Module 1: Time Control
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Time Control");
        // Slider from 0.0x (Freeze frame) to 5.0x (Fast-forward)
        ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 5.0f, "%.2fx");
        if (ImGui::Button("Reset Time")) {
            timeScale = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            timeScale = 0.0f;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Module 2: Entity spawner
        if (ImGui::Button("Shoot Planet / Black Hole!")) {
            // Get coordinates and viewing direction from the camera
            glm::vec3 camPos = renderer.getCameraPos();
            glm::vec3 camFront = renderer.getCameraFront();

            Vector3 pos(camPos.x, camPos.y, camPos.z);
            // Lauch velocity = Direction * Speed
            Vector3 vel(camFront.x * shootSpeed, camFront.y * shootSpeed, camFront.z * shootSpeed);

            // Load the new celestial body into the SoA system
            sim.addBody(newPlaneMass, pos, vel);
        }

        // Module 3: Interacting via Raycasting
        // Planets can only be selected by clicking while holding the ALT key (mouse unlocked)
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            // Check for left mouse clicks
            // and ensure the mouse does not accidentally click on ImGui windows
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
                ImVec2 mousePos = ImGui::GetMousePos();
                glm::vec3 rayDir = renderer.getRayDirection(mousePos.x, mousePos.y);

                // Lauch laser ray into space
                selectedEntity = raycast(renderer.getCameraPos(), rayDir, sim.getPositions(), sim.getMasses());
            }
        }

        // If a planet is currently selected, display the detailed information panel
        if (selectedEntity != -1 && selectedEntity < sim.getBodyCount()) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Target Lockeed: Entity #%d", selectedEntity);

            double m = sim.getMasses()[selectedEntity];
            Vector3 p = sim.getPositions()[selectedEntity];

            ImGui::Text("Mass: %.2f", m);
            ImGui::Text("Position: (%.0f, %.0f, %.0f)", p.x, p.y, p.z);

            // Destroy a planet button
            if (ImGui::Button("Destroy Entity", ImVec2(-1, 30))) {
                sim.removeBody(selectedEntity);
                selectedEntity = -1;
            }
        }

        ImGui::End();

        // UI rendering overlaid on 3D graphics
        renderer.endUI();
        
        renderer.swapBuffers();
    }

    std::cout << "Simulation has ended" << std::endl;
    return 0;
}