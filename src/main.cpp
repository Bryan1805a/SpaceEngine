#include <iostream>
#include <cmath>
#include <string>
#include <set>
#include <ctime>
#include <imgui.h>
#include <Math/Vector3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Simulation/Simulation.hpp>
#include <Graphics/Renderer.hpp>

// Gravitational constant in AU^3 / (M_earth * Year^2) and the Sun's mass in M_earth.
// Together they set the scale of every orbit: v_circular = sqrt(G*M_sun / a).
constexpr double SM_GRAVITY = 0.000118549;
constexpr double SM_SUN_MASS = 333000.0;


void DrawAcrylicBackground(const Graphics::Renderer& renderer) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p_min = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

    float sw = (float)renderer.getWidth();
    float sh = (float)renderer.getHeight();

    ImVec2 uv_min(p_min.x / sw, 1.0f - p_min.y / sh);
    ImVec2 uv_max(p_max.x / sw, 1.0f - p_max.y / sh);
    
    drawList->AddImage((ImTextureID)(intptr_t)renderer.getUIBlurTexture(), p_min, p_max, uv_min, uv_max);
}

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

const char* bodyTypeName(Simulation::BodyType type) {
    switch (type) {
        case Simulation::BodyType::STAR: return "Star";
        case Simulation::BodyType::ROCKY_PLANET: return "Rocky Planet";
        case Simulation::BodyType::GAS_GIANT: return "Gas Giant";
        case Simulation::BodyType::ICE_MOON: return "Ice Moon";
        case Simulation::BodyType::ASTEROID: return "Asteroid";
        default: return "Unknown";
    }
}

// EVE-style accent colour per body type
ImVec4 bodyTypeColor(Simulation::BodyType type) {
    switch (type) {
        case Simulation::BodyType::STAR: return ImVec4(1.00f, 0.80f, 0.30f, 1.0f);
        case Simulation::BodyType::ROCKY_PLANET: return ImVec4(0.60f, 0.80f, 0.90f, 1.0f);
        case Simulation::BodyType::GAS_GIANT: return ImVec4(0.55f, 0.85f, 0.70f, 1.0f);
        case Simulation::BodyType::ICE_MOON: return ImVec4(0.70f, 0.85f, 1.00f, 1.0f);
        case Simulation::BodyType::ASTEROID: return ImVec4(0.55f, 0.55f, 0.60f, 1.0f);
        default: return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
    }
}

// World units are AU. 1 AU ~= 1.496e8 km.
std::string formatDistance(double au) {
    if (au < 0.001) {
        return std::to_string(au * 1.496e8).substr(0, 6) + " km";
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3f AU", au);
    return buf;
}

int main() {
    // Reference frame:
    // G = 0.000118549 (AU^3 / (M_earth * Year^2))
    // dt = 0.001 (Approximately 0.365 days)
    Simulation::System sim(SM_GRAVITY, 0.001);

    struct OrbitalElements {
        Vector3 position;
        Vector3 velocity;
    };

    auto calculateOrbit = [&](double semiMajorAxis, double incDeg, double lanDeg, double centralMass = SM_SUN_MASS) {
        double v = std::sqrt(SM_GRAVITY * centralMass / semiMajorAxis);
        
        // Start with flat orbit on +X axis
        glm::vec3 pos(semiMajorAxis, 0.0, 0.0);
        glm::vec3 vel(0.0, 0.0, -v);
        
        // 1. Incline orbit around X axis
        float incRad = glm::radians((float)incDeg);
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), incRad, glm::vec3(1.0f, 0.0f, 0.0f));
        pos = glm::vec3(rotX * glm::vec4(pos, 1.0f));
        vel = glm::vec3(rotX * glm::vec4(vel, 0.0f));

        // 2. Rotate ascending node around Y axis (up)
        float lanRad = glm::radians((float)lanDeg);
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), lanRad, glm::vec3(0.0f, 1.0f, 0.0f));
        pos = glm::vec3(rotY * glm::vec4(pos, 1.0f));
        vel = glm::vec3(rotY * glm::vec4(vel, 0.0f));

        return OrbitalElements{Vector3(pos.x, pos.y, pos.z), Vector3(vel.x, vel.y, vel.z)};
    };

    // Visual scaling factors for better visibility
    const double SCALE_SUN = 20.0;
    const double SCALE_ROCKY = 500.0;
    const double SCALE_GAS = 100.0;

    // Init Sun
    Simulation::PlanetDesc sun;
    sun.name = "Sol";
    sun.type = Simulation::BodyType::STAR;
    sun.mass = SM_SUN_MASS;
    sun.position = Vector3::Zero;
    sun.velocity = Vector3::Zero;
    sun.angularVelocity = glm::vec3(0.0f, 10.0f, 0.0f);
    sun.radius = 0.0046505 * SCALE_SUN; // 695,700 km
    sim.addBody(sun);

    // Init Mercury
    Simulation::PlanetDesc mercury;
    mercury.name = "Mercury";
    mercury.type = Simulation::BodyType::ROCKY_PLANET;
    mercury.mass = 0.055;
    OrbitalElements orbMerc = calculateOrbit(0.387, 7.00, 48.33);
    mercury.position = orbMerc.position;
    mercury.velocity = orbMerc.velocity;
    mercury.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f);
    mercury.albedo = 0.12;
    mercury.greenhouse = 1.0;
    mercury.radius = 1.6308e-5 * SCALE_ROCKY; // 2,439.7 km
    sim.addBody(mercury);

    // Init Venus
    Simulation::PlanetDesc venus;
    venus.name = "Venus";
    venus.type = Simulation::BodyType::ROCKY_PLANET;
    venus.mass = 0.815;
    OrbitalElements orbVen = calculateOrbit(0.723, 3.39, 76.68);
    venus.position = orbVen.position;
    venus.velocity = orbVen.velocity;
    venus.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f);
    venus.albedo = 0.65;                   // Shiny acid clouds
    venus.greenhouse = 2.3;                // Runaway greenhouse -> ~700K
    venus.radius = 4.0454e-5 * SCALE_ROCKY; // 6,051.8 km
    sim.addBody(venus);

    // Init Earth
    Simulation::PlanetDesc earth;
    earth.name = "Earth";
    earth.type = Simulation::BodyType::ROCKY_PLANET;
    earth.mass = 1.0;
    OrbitalElements orbEarth = calculateOrbit(1.0, 0.0, -11.26);
    earth.position = orbEarth.position;
    earth.velocity = orbEarth.velocity;
    earth.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f); // Rotate 365/year
    earth.albedo = 0.30;
    earth.greenhouse = 1.13; // 255K -> 288K
    earth.radius = 4.2588e-5 * SCALE_ROCKY; // 6,371 km
    sim.addBody(earth);

    // Init Mars
    Simulation::PlanetDesc mars;
    mars.name = "Mars";
    mars.type = Simulation::BodyType::ROCKY_PLANET;
    mars.mass = 0.107;
    OrbitalElements orbMars = calculateOrbit(1.524, 1.85, 49.58);
    mars.position = orbMars.position;
    mars.velocity = orbMars.velocity;
    mars.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f);
    mars.albedo = 0.25;
    mars.greenhouse = 1.01;
    mars.radius = 2.2657e-5 * SCALE_ROCKY; // 3,389.5 km
    sim.addBody(mars);

    // Init Jupiter
    Simulation::PlanetDesc jupiter;
    jupiter.name = "Jupiter";
    jupiter.type = Simulation::BodyType::GAS_GIANT;
    jupiter.mass = 317.8;
    OrbitalElements orbJup = calculateOrbit(5.204, 1.30, 100.55);
    jupiter.position = orbJup.position;
    jupiter.velocity = orbJup.velocity;
    jupiter.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f);
    jupiter.albedo = 0.52; // Ammonia clouds are highly reflective
    jupiter.greenhouse = 1.0;
    jupiter.radius = 4.6733e-4 * SCALE_GAS; // 69,911 km
    sim.addBody(jupiter);

    // Init Saturn
    Simulation::PlanetDesc saturn;
    saturn.name = "Saturn";
    saturn.type = Simulation::BodyType::GAS_GIANT;
    saturn.mass = 95.16;
    OrbitalElements orbSat = calculateOrbit(9.58, 2.48, 113.72);
    saturn.position = orbSat.position;
    saturn.velocity = orbSat.velocity;
    saturn.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f);
    saturn.albedo = 0.47;
    saturn.greenhouse = 1.0;
    saturn.radius = 3.8926e-4 * SCALE_GAS; // 58,232 km
    sim.addBody(saturn);

    // Init Uranus
    Simulation::PlanetDesc uranus;
    uranus.name = "Uranus";
    uranus.type = Simulation::BodyType::GAS_GIANT;
    uranus.mass = 14.54;
    OrbitalElements orbUr = calculateOrbit(19.2, 0.77, 74.23);
    uranus.position = orbUr.position;
    uranus.velocity = orbUr.velocity;
    uranus.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f);
    uranus.albedo = 0.51;
    uranus.greenhouse = 1.0;
    uranus.radius = 1.6953e-4 * SCALE_GAS; // 25,362 km
    sim.addBody(uranus);

    // Init Neptune
    Simulation::PlanetDesc neptune;
    neptune.name = "Neptune";
    neptune.type = Simulation::BodyType::GAS_GIANT;
    neptune.mass = 17.15;
    OrbitalElements orbNep = calculateOrbit(30.05, 1.77, 131.72);
    neptune.position = orbNep.position;
    neptune.velocity = orbNep.velocity;
    neptune.angularVelocity = glm::vec3(0.0f, 365.0f, 0.0f);
    neptune.albedo = 0.41;
    neptune.greenhouse = 1.0;
    neptune.radius = 1.6459e-4 * SCALE_GAS; // 24,622 km
    sim.addBody(neptune);

    // Init Moon
    Simulation::PlanetDesc moon;
    moon.name = "Moon";
    moon.type = Simulation::BodyType::ROCKY_PLANET;
    moon.mass = 0.0123;
    OrbitalElements orbMoon = calculateOrbit(0.00257, 5.14, 125.08, earth.mass);
    moon.position = earth.position + orbMoon.position;
    moon.velocity = earth.velocity + orbMoon.velocity;
    moon.angularVelocity = glm::vec3(0.0f, 27.3f, 0.0f); // Tidally locked
    moon.albedo = 0.12;
    moon.greenhouse = 1.0;
    moon.radius = 1.159e-5 * SCALE_ROCKY; // 1,737 km
    sim.addBody(moon);

    // Init graphics
    Graphics::Renderer renderer(1280, 720, "SpaceEngine");
    std::cout << "Initialised 1000 asteroids" << std::endl;
    std::cout << "Press X to quit" << std::endl;
    std::cout << "Press F11 to toggle fullscreen" << std::endl;
    std::cout << "Press TAB to toggle the UI cursor (or hold ALT while flying)" << std::endl;

    // Delta Time vars
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // UI status vars
    float timeScale = 1.0f;         // Time speed
    float shootSpeed = 300.0f;      // Lauch velocity
    const double baseDt = 0.001;    // Base time step (~0.365 days)

    bool showOverview = true;
    bool showSelected = false;
    bool showNav = true;
    bool showSystemInfo = true;
    bool showHud = true;

    // Main loop
    bool showUI = true;
    bool hWasPressed = false;
    std::set<int> selectedEntities;
    while (!renderer.shouldClose()) {
        // Update Delta Time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Operating System Event Handling and Movement Logic
        renderer.pollEvents();
        renderer.processInput(deltaTime);

        // Update camera
        renderer.updateCameraTracking(sim.getPositions());

        // Apply time scale for physics
        sim.setDt(baseDt * timeScale);
        sim.step();
        renderer.clear();

        // Upload objects into GPU
        renderer.draw(sim.getBodyCount(), sim.getPositions(), sim.getVelocities(), sim.getRadii(),
                      sim.getOrientations(), sim.getTypes(), sim.getTemperatures());

        // Draw UI
        renderer.beginUI();

        float dt = ImGui::GetIO().DeltaTime;
        (void)dt;

        const std::vector<std::string>& bNames = sim.getNames();
        const std::vector<Vector3>& bPos = sim.getPositions();
        const std::vector<Vector3>& bVel = sim.getVelocities();
        const std::vector<double>& bMass = sim.getMasses();
        const std::vector<double>& bRadii = sim.getRadii();
        const std::vector<double>& bTemp = sim.getTemperatures();
        const std::vector<Simulation::BodyType>& bTypes = sim.getTypes();

        // Handle H key toggle for UI
        bool hPressed = ImGui::IsKeyDown(ImGuiKey_H);
        if (hPressed && !hWasPressed) {
            showUI = !showUI;
        }
        hWasPressed = hPressed;

        if (showUI) {
            // ============ SPACE ENGINE UI ============
            // LEFT PANEL (Settings)
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("SpaceEngine Settings", nullptr, ImGuiWindowFlags_NoCollapse);
            DrawAcrylicBackground(renderer);
            
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Simulation Time");
            ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 5.0f, "%.2fx");
            if (ImGui::Button("Reset Time")) timeScale = 1.0f;
            ImGui::SameLine();
            if (ImGui::Button("Pause")) timeScale = 0.0f;
            
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Camera");
            ImGui::SliderFloat("Speed", &renderer.getCameraSpeed(), 1.0f, 500.0f, "%.1f units/s");
            
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "System Information");
            ImGui::Text("Bodies: %zu", sim.getBodyCount());
            ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            
            if (ImGui::Button("Spawn Black Hole", ImVec2(-1, 0))) {
                glm::vec3 camPos = renderer.getCameraPos();
                glm::vec3 camFront = renderer.getCameraFront();
                Vector3 pos(camPos.x, camPos.y, camPos.z);
                Vector3 vel(camFront.x * shootSpeed, camFront.y * shootSpeed, camFront.z * shootSpeed);
                Simulation::PlanetDesc blackhole;
                blackhole.name = "Spawn #" + std::to_string(sim.getBodyCount());
                blackhole.type = Simulation::BodyType::STAR;
                blackhole.mass = 5000;
                blackhole.position = pos;
                blackhole.velocity = vel;
                blackhole.radius = 0.20;
                sim.addBody(blackhole);
            }
            ImGui::End();

            // RIGHT PANEL (Celestial Bodies)
            ImGui::SetNextWindowPos(ImVec2((float)renderer.getWidth() - 310, 10), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, (float)renderer.getHeight() - 20), ImGuiCond_Always);
            ImGui::Begin("Celestial Bodies", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
            DrawAcrylicBackground(renderer);

            // Right click on empty space in the panel
            if (ImGui::BeginPopupContextWindow("PanelContextMenu")) {
                if (ImGui::MenuItem("Select All")) {
                    for (size_t i = 0; i < bNames.size(); ++i) {
                        selectedEntities.insert((int)i);
                    }
                }
                if (ImGui::MenuItem("Deselect All")) {
                    selectedEntities.clear();
                }
                ImGui::EndPopup();
            }

            ImGui::BeginChild("List", ImVec2(0, 0), true);
            for (size_t i = 0; i < bNames.size(); ++i) {
                ImGui::PushID((int)i);
                
                ImVec4 color = bodyTypeColor(bTypes[i]);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                bool isSelected = selectedEntities.count((int)i) > 0;
                
                if (ImGui::Selectable(bNames[i].c_str(), isSelected)) {
                    if (!ImGui::GetIO().KeyCtrl) {
                        selectedEntities.clear();
                    }
                    if (isSelected && ImGui::GetIO().KeyCtrl) {
                        selectedEntities.erase((int)i);
                    } else {
                        selectedEntities.insert((int)i);
                    }
                }
                ImGui::PopStyleColor();

                // Right-click context menu
                if (ImGui::BeginPopupContextItem("Context Menu")) {
                    if (selectedEntities.find((int)i) == selectedEntities.end()) {
                        selectedEntities.clear();
                        selectedEntities.insert((int)i);
                    }
                    
                    ImGui::TextColored(color, "%s", bNames[i].c_str());
                    ImGui::Separator();
                    
                    if (ImGui::MenuItem("Lock & Focus Camera")) {
                        float planetRadius = (float)bRadii[i];
                        float viewDistance = std::max(planetRadius * 6.0f, planetRadius + 1.0e-4f);
                        renderer.lockTarget((int)i, viewDistance, planetRadius);
                    }
                    
                    if (renderer.isTargetLocked() && renderer.getLockedTargetIndex() == (int)i) {
                        if (ImGui::MenuItem("Unlock Camera")) {
                            renderer.unlockTarget();
                        }
                    }
                    
                    if (ImGui::MenuItem("Destroy Body")) {
                        if (renderer.isTargetLocked() && renderer.getLockedTargetIndex() == (int)i) {
                            renderer.unlockTarget();
                        }
                        sim.removeBody(i);
                        selectedEntities.erase((int)i);
                    }
                    ImGui::EndPopup();
                }
                
                // Show basic info if selected
                if (isSelected) {
                    ImGui::Indent();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Type: %s", bodyTypeName(bTypes[i]));
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Mass: %.2f M_E", bMass[i]);
                    float distAU = glm::length(glm::vec3((float)bPos[i].x, (float)bPos[i].y, (float)bPos[i].z) - renderer.getCameraPos());
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Distance: %.3f AU", distAU);
                    ImGui::Unindent();
                }
                
                ImGui::PopID();
            }
            ImGui::EndChild();
            ImGui::End();

            // Draw HUD reticles in the background draw list
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            for (int idx : selectedEntities) {
                if (idx < 0 || idx >= (int)bPos.size()) continue;
                glm::vec2 screenPos;
                float distToCam;
                glm::vec3 p((float)bPos[idx].x, (float)bPos[idx].y, (float)bPos[idx].z);
                if (renderer.worldToScreen(p, screenPos, distToCam)) {
                    // Draw square reticle
                    float size = 20.0f;
                    ImU32 reticleColor = IM_COL32(100, 255, 100, 200); // Greenish
                    
                    // Top-left
                    drawList->AddLine(ImVec2(screenPos.x - size, screenPos.y - size), ImVec2(screenPos.x - size + 8, screenPos.y - size), reticleColor, 2.0f);
                    drawList->AddLine(ImVec2(screenPos.x - size, screenPos.y - size), ImVec2(screenPos.x - size, screenPos.y - size + 8), reticleColor, 2.0f);
                    // Top-right
                    drawList->AddLine(ImVec2(screenPos.x + size, screenPos.y - size), ImVec2(screenPos.x + size - 8, screenPos.y - size), reticleColor, 2.0f);
                    drawList->AddLine(ImVec2(screenPos.x + size, screenPos.y - size), ImVec2(screenPos.x + size, screenPos.y - size + 8), reticleColor, 2.0f);
                    // Bottom-left
                    drawList->AddLine(ImVec2(screenPos.x - size, screenPos.y + size), ImVec2(screenPos.x - size + 8, screenPos.y + size), reticleColor, 2.0f);
                    drawList->AddLine(ImVec2(screenPos.x - size, screenPos.y + size), ImVec2(screenPos.x - size, screenPos.y + size - 8), reticleColor, 2.0f);
                    // Bottom-right
                    drawList->AddLine(ImVec2(screenPos.x + size, screenPos.y + size), ImVec2(screenPos.x + size - 8, screenPos.y + size), reticleColor, 2.0f);
                    drawList->AddLine(ImVec2(screenPos.x + size, screenPos.y + size), ImVec2(screenPos.x + size, screenPos.y + size - 8), reticleColor, 2.0f);
                    
                    // Circular HUD indicator in center
                    drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), 4.0f, IM_COL32(100, 255, 100, 255), 12, 1.5f);
                    drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), 12.0f, IM_COL32(100, 255, 100, 100), 24, 1.0f);
                    
                    // Name label
                    drawList->AddText(ImVec2(screenPos.x + size + 4, screenPos.y - size), IM_COL32(200, 255, 200, 255), bNames[idx].c_str());
                    
                    // Distance label
                    char distStr[32];
                    snprintf(distStr, sizeof(distStr), "%.3f AU", distToCam);
                    drawList->AddText(ImVec2(screenPos.x + size + 4, screenPos.y - size + 14), IM_COL32(150, 200, 150, 200), distStr);
                }
            }
        }

        renderer.endUI();

        renderer.swapBuffers();
    }

    std::cout << "Simulation has ended" << std::endl;
    return 0;
}
