
import re

with open("src/main.cpp", "r") as f:
    lines = f.readlines()

start_idx = -1
end_idx = -1

for i, line in enumerate(lines):
    if "// ============  LEFT NEOCOM DOCK  ============" in line:
        start_idx = i
    if "renderer.endUI();" in line:
        end_idx = i

new_ui = """        // ============ SPACE ENGINE UI ============
        // LEFT PANEL (Settings)
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("SpaceEngine Settings", nullptr, ImGuiWindowFlags_NoCollapse);
        
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

        ImGui::BeginChild("List", ImVec2(0, 0), true);
        for (size_t i = 0; i < bNames.size(); ++i) {
            ImGui::PushID((int)i);
            
            ImVec4 color = bodyTypeColor(bTypes[i]);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            bool isSelected = (selectedEntity == (int)i);
            
            if (ImGui::Selectable(bNames[i].c_str(), isSelected)) {
                selectedEntity = (int)i;
            }
            ImGui::PopStyleColor();

            // Right-click context menu
            if (ImGui::BeginPopupContextItem("Context Menu")) {
                selectedEntity = (int)i;
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
                    selectedEntity = -1;
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
"""

if start_idx != -1 and end_idx != -1:
    new_lines = lines[:start_idx] + [new_ui] + lines[end_idx:]
    with open("src/main.cpp", "w") as f:
        f.writelines(new_lines)
    print("Updated main.cpp")
else:
    print("Could not find start or end index")
