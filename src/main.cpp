#include <iostream>
#include <cmath>
#include <string>
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
    int selectedEntity = -1;
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

        // ============  LEFT NEOCOM DOCK  ============
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(52, (float)renderer.getHeight()), ImGuiCond_Always);
            ImGui::Begin("##neocom", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.22f, 0.30f, 0.90f));

            // Top: system / menu icon
            if (ImGui::Button("☰", ImVec2(36, 30))) showSystemInfo = !showSystemInfo;
            ImGui::Dummy(ImVec2(0, 8));

            if (ImGui::Button("◉", ImVec2(36, 30))) { showOverview = !showOverview; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overview (O)");
            ImGui::Dummy(ImVec2(0, 4));

            if (ImGui::Button("◎", ImVec2(36, 30))) { showSelected = !showSelected; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Selected Item");
            ImGui::Dummy(ImVec2(0, 4));

            if (ImGui::Button("➤", ImVec2(36, 30))) { showNav = !showNav; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Navigation / Time");
            ImGui::Dummy(ImVec2(0, 4));

            if (ImGui::Button("◎", ImVec2(36, 30))) { showHud = !showHud; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tactical HUD");

            ImGui::PopStyleColor(2);

            // Bottom: system clock
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 8));

            time_t now = time(nullptr);
            struct tm tmbuf;
            localtime_s(&tmbuf, &now);
            char clockStr[16];
            strftime(clockStr, sizeof(clockStr), "%H:%M", &tmbuf);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.75f, 0.95f, 1.0f));
            ImGui::SetCursorPosX(8);
            ImGui::Text("%s", clockStr);
            ImGui::PopStyleColor();

            ImGui::End();
        }

        // SYSTEM INFO (top-left)
        if (showSystemInfo) {
            ImGui::SetNextWindowPos(ImVec2(62, 8), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(270, 0), ImGuiCond_FirstUseEver);
            ImGui::Begin("Solar System // Sol", nullptr,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.00f, 1.0f), "Jita IV - Moon 6 - General");
            ImGui::Separator();
            ImGui::Text("Bodies: %zu", sim.getBodyCount());
            ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
            ImGui::Text("Velocity: %.2f units/s", renderer.getCameraSpeed());

            ImGui::Spacing();
            if (ImGui::TreeNode("Ecosystem")) {
                ImGui::Text("Sol has no signatures or anomalies currently.");
                ImGui::TreePop();
            }
            ImGui::End();
        }

        // OVERVIEW (right dock)
        if (showOverview) {
            float w = 330.0f;
            ImGui::SetNextWindowPos(ImVec2((float)renderer.getWidth() - w - 8, 8), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(w, (float)renderer.getHeight() - 16), ImGuiCond_Always);
            ImGui::Begin("Overview (Solar System)", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::TextColored(ImVec4(0.40f, 0.60f, 0.80f, 1.0f), "Jita IV - Moon 6 - Jita IV Moon 6 - Jita II - Jita IV - Jita VI - Jita VIII");
            ImGui::Separator();

            // Simulated EVE tabs
            if (ImGui::BeginTabBar("overviewTabs")) {
                if (ImGui::BeginTabItem("General")) { ImGui::EndTabItem(); }
                if (ImGui::BeginTabItem("Mining")) { ImGui::EndTabItem(); }
                if (ImGui::BeginTabItem("Warp To")) { ImGui::EndTabItem(); }
                if (ImGui::BeginTabItem("RPG")) { ImGui::EndTabItem(); }
                ImGui::EndTabBar();
            }

            static char searchBuf[128] = "";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 28.0f);
            ImGui::InputTextWithHint("##search", "Search...", searchBuf, sizeof(searchBuf));

            if (ImGui::BeginTable("overviewTable", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.25f);
                ImGui::TableSetupColumn("Dist", ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGui::TableSetupColumn("Vel", ImGuiTableColumnFlags_WidthStretch, 0.10f);
                ImGui::TableHeadersRow();

                std::string lowerQuery(searchBuf);
                for (size_t i = 0; i < bNames.size(); ++i) {
                    std::string nameLower = bNames[i];
                    for (auto& c : nameLower) c = (char)tolower(c);
                    if (!lowerQuery.empty() && nameLower.find(lowerQuery) == std::string::npos) continue;

                    glm::vec3 cpos((float)bPos[i].x, (float)bPos[i].y, (float)bPos[i].z);
                    float distAU = glm::length(cpos - renderer.getCameraPos());

                    ImGui::PushID((int)i);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImVec4 c = bodyTypeColor(bTypes[i]);
                    ImGui::PushStyleColor(ImGuiCol_Text, c);
                    ImGui::Text("%s", bNames[i].c_str());
                    ImGui::PopStyleColor();
                    if (ImGui::IsItemClicked()) selectedEntity = (int)i;
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", bodyTypeName(bTypes[i]));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(bodyTypeName(bTypes[i]));

                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", distAU);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", glm::length(glm::vec3((float)bVel[i].x, (float)bVel[i].y, (float)bVel[i].z)));
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        // SELECTED ITEM (top-right)
        if (showSelected) {
            ImGui::SetNextWindowPos(ImVec2((float)renderer.getWidth() - 340.0f, 8), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(332, 0), ImGuiCond_Always);
            ImGui::Begin("Selected Item", &showSelected, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            if (selectedEntity < 0 || selectedEntity >= (int)bNames.size()) {
                ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "No target selected.");
                ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "Select a body from the Overview or hold ALT to click in space.");
            }
            else {
                int idx = selectedEntity;
                ImVec4 c = bodyTypeColor(bTypes[idx]);
                ImGui::TextColored(c, "%s", bNames[idx].c_str());
                ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "%s", bodyTypeName(bTypes[idx]));
                ImGui::Separator();

                ImGui::Text("Distance: %s", formatDistance(glm::length(glm::vec3((float)bPos[idx].x, (float)bPos[idx].y, (float)bPos[idx].z) - renderer.getCameraPos())).c_str());
                ImGui::Text("Velocity: %.2f km/s", glm::length(glm::vec3((float)bVel[idx].x, (float)bVel[idx].y, (float)bVel[idx].z)) * 29.78);

                ImGui::Spacing();
                if (ImGui::BeginTable("selSpec", 2, ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableSetupColumn("Attribute", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                    ImGui::TableHeadersRow();

                    ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("Mass");
                    ImGui::TableNextColumn(); ImGui::Text("%.2f M_E", bMass[idx]);
                    ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("Radius");
                    ImGui::TableNextColumn(); ImGui::Text("%.3f AU", bRadii[idx]);
                    ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("Temperature");
                    ImGui::TableNextColumn();
                    float tmp = (float)bTemp[idx];
                    if (tmp > 400.0f) ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%.1f K", tmp);
                    else if (tmp < 200.0f) ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "%.1f K", tmp);
                    else ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%.1f K", tmp);
                    ImGui::EndTable();
                }

                ImGui::Spacing();
                if (ImGui::Button("Track Orbit (Follow)", ImVec2(-1, 30))) {
                    // Start the orbit a few radii from the planet's surface; with true
                    // scale that can be a tiny distance, which the camera handles via
                    // its log zoom + adaptive near/far planes.
                    float planetRadius = (float)bRadii[idx];
                    float viewDistance = std::max(planetRadius * 6.0f, planetRadius + 1.0e-4f);
                    renderer.lockTarget(idx, viewDistance, planetRadius);
                }
                if (renderer.isTargetLocked() && renderer.getLockedTargetIndex() == idx) {
                    if (ImGui::Button("Unlock Camera", ImVec2(-1, 30))) {
                        renderer.unlockTarget();
                    }
                }
                if (ImGui::Button("Destroy Entity", ImVec2(-1, 30))) {
                    if (renderer.isTargetLocked() && renderer.getLockedTargetIndex() == idx) {
                        renderer.unlockTarget();
                    }
                    sim.removeBody(idx);
                    selectedEntity = -1;
                }
            }
            ImGui::End();
        }

        //NAVIGATION / TIME (bottom-left)
        if (showNav) {
            ImGui::SetNextWindowPos(ImVec2(62, (float)renderer.getHeight() - 150), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(290, 142), ImGuiCond_Always);
            ImGui::Begin("Navigation", &showNav,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.00f, 1.0f), "Time Control");
            ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 5.0f, "%.2fx");
            if (ImGui::Button("Reset Time", ImVec2(-1, 24))) timeScale = 1.0f;
            ImGui::SameLine();
            if (ImGui::Button("Pause", ImVec2(-1, 24))) timeScale = 0.0f;

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Navigation");
            ImGui::SliderFloat("Camera Speed", &renderer.getCameraSpeed(), 1.0f, 500.0f, "%.1f units/s");

            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Shoot Planet / Black Hole", ImVec2(-1, 26))) {
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
        }

        // TACTICAL HUD (bottom-center)
        if (showHud) {
            ImGui::SetNextWindowPos(ImVec2((float)renderer.getWidth() * 0.5f - 120.0f, (float)renderer.getHeight() - 78.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(240, 74), ImGuiCond_Always);
            ImGui::Begin("##hud", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

            // Speedometer
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetWindowPos();
            ImVec2 c = ImVec2(p.x + 30, p.y + 37);
            float radius = 26.0f;

            // Outer ring
            dl->AddCircle(c, radius, IM_COL32(60, 130, 170, 120), 48, 2.0f);
            dl->AddCircle(c, radius * 0.68f, IM_COL32(30, 70, 95, 120), 48, 1.0f);

            // Needle based on camera speed
            float speed = renderer.getCameraSpeed();
            float angle = glm::radians(-135.0f) + (speed / 500.0f) * glm::radians(270.0f);
            ImVec2 needle = ImVec2(c.x + cosf(angle) * radius * 0.6f, c.y + sinf(angle) * radius * 0.6f);
            dl->AddLine(c, needle, IM_COL32(80, 200, 255, 255), 2.0f);
            dl->AddCircleFilled(c, 3.0f, IM_COL32(120, 220, 255, 255));

            // Readouts
            ImGui::SetCursorPos(ImVec2(72, 12));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.80f, 1.00f, 1.0f));
            ImGui::Text("VEL");
            ImGui::PopStyleColor();
            ImGui::SetCursorPos(ImVec2(72, 26));
            char velBuf[32];
            snprintf(velBuf, sizeof(velBuf), "%.1f", speed * 29.78);
            ImGui::Text("%s km/s", velBuf);

            ImGui::SetCursorPos(ImVec2(150, 12));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.80f, 1.00f, 1.0f));
            ImGui::Text("HDG");
            ImGui::PopStyleColor();
            ImGui::SetCursorPos(ImVec2(150, 26));
            ImGui::Text("%.0f°", renderer.getCameraYaw());

            ImGui::End();
        }

        // IN-VIEWPORT 3D BRACKETS
        {
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            for (size_t i = 0; i < bPos.size(); ++i) {
                glm::vec2 screen;
                float dist;
                glm::vec3 world((float)bPos[i].x, (float)bPos[i].y, (float)bPos[i].z);
                if (!renderer.worldToScreen(world, screen, dist)) continue;

                bool isSelected = ((int)i == selectedEntity);
                bool isLocked = renderer.isTargetLocked() && renderer.getLockedTargetIndex() == (int)i;

                ImU32 col = isLocked ? IM_COL32(255, 120, 60, 255) : (isSelected ? IM_COL32(120, 220, 255, 255) : IM_COL32(90, 150, 180, 180));

                float half = isSelected ? 16.0f : 12.0f;
                float t = 5.0f; // corner tick length

                ImVec2 p0(screen.x - half, screen.y - half);
                ImVec2 p1(screen.x + half, screen.y + half);
                // Corner brackets
                fg->AddLine(p0, ImVec2(p0.x + t, p0.y), col, 1.5f);
                fg->AddLine(p0, ImVec2(p0.x, p0.y + t), col, 1.5f);
                fg->AddLine(ImVec2(p1.x - t, p1.y), p1, col, 1.5f);
                fg->AddLine(ImVec2(p1.x, p1.y - t), p1, col, 1.5f);
                fg->AddLine(ImVec2(p0.x, p1.y), ImVec2(p0.x, p1.y - t), col, 1.5f);
                fg->AddLine(ImVec2(p0.x, p1.y), ImVec2(p0.x + t, p1.y), col, 1.5f);
                fg->AddLine(ImVec2(p1.x, p0.y), ImVec2(p1.x - t, p0.y), col, 1.5f);
                fg->AddLine(ImVec2(p1.x, p0.y), ImVec2(p1.x, p0.y + t), col, 1.5f);

                // Label + distance
                std::string label = bNames[i];
                char distBuf[24];
                snprintf(distBuf, sizeof(distBuf), "%.3f", dist);
                label += "  " + std::string(distBuf) + " AU";
                fg->AddText(ImVec2(screen.x - half, p0.y - 16.0f), IM_COL32(180, 220, 240, 200), label.c_str());
            }
        }

        renderer.endUI();

        renderer.swapBuffers();
    }

    std::cout << "Simulation has ended" << std::endl;
    return 0;
}
