#include <iostream>
#include <cmath>
#include <imgui.h>
#include <Math/Vector3.hpp>
#include <Simulation/Simulation.hpp>
#include <Graphics/Renderer.hpp>

int raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const std::vector<Vector3>& positions, const std::vector<double>& radii) {
    int hitIndex = -1;
    float minDistance = 999999.0f; // Find the nearest planet if the ray passes through multiple planets

    for (size_t i = 0; i < positions.size(); ++i) {
        glm::vec3 center((float)positions[i].x, (float)positions[i].y, (float)positions[i].z);
        float radius = (float)radii[i];

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
    // Reference frame:
    // G = 0.000118549 (AU^3 / (M_earth * Year^2))
    // dt = 0.001 (Approximately 0.365 days)
    Simulation::System sim(0.000118549, 0.001);

    // Init Sun
    Simulation::PlanetDesc sun;
    sun.type = Simulation::BodyType::STAR;
    sun.mass = 333000.0;
    sun.position = Vector3::Zero;
    sun.velocity = Vector3::Zero;
    sun.angularVelocity = glm::vec3(0.0f, 10.0f, 0.0f);
    sun.radius = 0.15;
    sim.addBody(sun);

    // Init Earth
    Simulation::PlanetDesc earth;
    earth.type = Simulation::BodyType::ROCKY_PLANET;
    earth.mass = 1.0;
    earth.position = Vector3(1.0, 0.0, 0.0); // At a distance of 1 AU from the Sun
    earth.velocity = Vector3(0.0, 0.0, -6.28318); // 2*PI AU/Year
    earth.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f); // Rotate 365/year
    earth.albedo = 0.30;
    earth.greenhouse = 1.13; // 255K -> 288K
    earth.radius = 0.04;
    sim.addBody(earth);
    
    // Init Mars
    Simulation::PlanetDesc mars;
    mars.type = Simulation::BodyType::ROCKY_PLANET;
    mars.mass = 0.107;
    mars.position = Vector3(1.524, 0.0, 0.0);
    mars.velocity = Vector3(0.0, 0.0, -5.026);
    mars.albedo = 0.25;
    mars.greenhouse = 1.01;
    mars.radius = 0.03;
    sim.addBody(mars);

    // Init Jupiter
    Simulation::PlanetDesc jupiter;
    jupiter.type = Simulation::BodyType::GAS_GIANT;
    jupiter.mass = 317.8;
    jupiter.position = Vector3(5.204, 0.0, 0.0);
    jupiter.velocity = Vector3(0.0, 0.0, -2.756);
    jupiter.albedo = 0.52; // Ammonia clouds are highly reflective
    jupiter.greenhouse = 1.0;
    jupiter.radius = 0.10;
    sim.addBody(jupiter);

    // Init graphics
    Graphics::Renderer renderer(1280, 720, "Galaxy Simulation");
    std::cout << "Initialised 1000 asteroids" << std::endl;
    std::cout << "Press X to quit" << std::endl;

    // Delta Time vars
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // UI status vars
    float timeScale = 1.0f;         // Time speed
    float shootSpeed = 300.0f;      // Lauch velocity
    const double baseDt = 0.001;    // Base time step (~0.365 days)

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
        sim.setDt(baseDt * timeScale);
        sim.step();
        renderer.clear();

        // Upload objects into GPU
        renderer.draw(sim.getBodyCount(), sim.getPositions(), sim.getRadii(), sim.getOrientations());

        // Draw UI
        renderer.beginUI();

        ImGui::Begin("Space Engine Control Panel");
        // Get the current coordinates and dimensions of the UI window
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 screenRes = ImVec2(1280.0f, 720.0f); // Display resolution

        // UV Calculation
        ImVec2 uv0 = ImVec2(winPos.x / screenRes.x, winPos.y / screenRes.y);
        ImVec2 uv1 = ImVec2((winPos.x + winSize.x) / screenRes.x, (winPos.y + winSize.y) / screenRes.y);

        // Invert Y-axis
        uv0.y = 1.0f - uv0.y;
        uv1.y = 1.0f - uv1.y;
        std::swap(uv0.y, uv1.y);

        // Apply blur glass effect
        ImGui::GetWindowDrawList()->AddImage(
            (void*)(intptr_t)renderer.getBlurredTexture(),
            winPos,
            ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
            uv0, uv1
        );

        // Add an ultra-thin gray/black overlay (Alpha = 0.2) 
        // over the frosted glass to make the text easier to read
        ImGui::GetWindowDrawList()->AddRectFilled(
            winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
            IM_COL32(10, 10, 10, 50)
        );

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

            // Init Black Hole
            Simulation::PlanetDesc blackhole;
            blackhole.type = Simulation::BodyType::STAR;
            blackhole.mass = 5000;
            blackhole.position = pos;
            blackhole.velocity = vel;
            blackhole.radius = 0.20;

            // Load the new celestial body into the SoA system
            sim.addBody(blackhole);
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
                selectedEntity = raycast(renderer.getCameraPos(), rayDir, sim.getPositions(), sim.getRadii());
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