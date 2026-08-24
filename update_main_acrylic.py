
import re

with open("src/main.cpp", "r") as f:
    content = f.read()

# Make sure we only insert once
if "DrawAcrylicBackground" not in content:
    acrylic_func = """
void DrawAcrylicBackground(const Graphics::Renderer& renderer) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p_min = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

    float sw = (float)renderer.getWidth();
    float sh = (float)renderer.getHeight();

    // OpenGL texture origin is bottom-left, ImGui origin is top-left
    ImVec2 uv_min(p_min.x / sw, 1.0f - p_min.y / sh);
    ImVec2 uv_max(p_max.x / sw, 1.0f - p_max.y / sh);
    
    drawList->AddImage((ImTextureID)(intptr_t)renderer.getUIBlurTexture(), p_min, p_max, uv_min, uv_max);
}
"""
    content = content.replace("int raycast", acrylic_func + "\nint raycast")

content = content.replace("ImGui::Begin(\"SpaceEngine Settings\", nullptr, ImGuiWindowFlags_NoCollapse);", 
"""ImGui::Begin("SpaceEngine Settings", nullptr, ImGuiWindowFlags_NoCollapse);
            DrawAcrylicBackground(renderer);""")

content = content.replace("ImGui::Begin(\"Celestial Bodies\", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);",
"""ImGui::Begin("Celestial Bodies", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
            DrawAcrylicBackground(renderer);""")

with open("src/main.cpp", "w") as f:
    f.write(content)

