
import re

# --- Patch Renderer.hpp ---
with open("include/Graphics/Renderer.hpp", "r") as f:
    hpp = f.read()

if "unsigned int getUIBlurTexture" not in hpp:
    hpp = hpp.replace(
        "GLFWwindow* getWindow() const {return window;}",
        "GLFWwindow* getWindow() const {return window;}\n            unsigned int getUIBlurTexture() const { return uiPingpongColorbuffers[0]; }"
    )
    with open("include/Graphics/Renderer.hpp", "w") as f:
        f.write(hpp)
    print("Patched Renderer.hpp")

# --- Patch main.cpp ---
with open("src/main.cpp", "r") as f:
    main_cpp = f.read()

# 1. Add #include <set>
if "<set>" not in main_cpp:
    main_cpp = main_cpp.replace("#include <string>", "#include <string>\n#include <set>")

# 2. Modify globals
main_cpp = re.sub(
    r"    int selectedEntity = -1;",
    "    bool showUI = true;\n    bool hWasPressed = false;\n    std::set<int> selectedEntities;",
    main_cpp
)

# 3. Add DrawAcrylicBackground
acrylic_func = """
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
"""
if "DrawAcrylicBackground" not in main_cpp:
    main_cpp = main_cpp.replace("int raycast", acrylic_func + "\nint raycast")

# 4. Remove mouse clicking selection logic
main_cpp = re.sub(
    r"        if \(!ImGui::GetIO\(\)\.WantCaptureMouse\) \{.*?selectedEntity = hitIndex;\n            }\n        }",
    """        if (!ImGui::GetIO().WantCaptureMouse) {
            // Disabled raycast selection as requested to prevent clutter
        }""",
    main_cpp,
    flags=re.DOTALL
)

# 5. Replace the ENTIRE UI block
start_marker = "        // ============ SPACE ENGINE UI ============"
end_marker = "        renderer.endUI();"

start_idx = main_cpp.find(start_marker)
end_idx = main_cpp.find(end_marker, start_idx)

ui_replacement = """        // Handle H key toggle for UI
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
\n"""

main_cpp = main_cpp[:start_idx] + ui_replacement + main_cpp[end_idx:]

with open("src/main.cpp", "w") as f:
    f.write(main_cpp)
print("Patched main.cpp")
