
import re

with open("src/Graphics/Renderer.cpp", "r") as f:
    content = f.read()

content = content.replace("style.Colors[ImGuiCol_WindowBg]    = ImVec4(0.02f, 0.02f, 0.02f, 0.60f);", 
                          "style.Colors[ImGuiCol_WindowBg]    = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);")

content = content.replace("style.Colors[ImGuiCol_PopupBg]     = ImVec4(0.05f, 0.05f, 0.05f, 0.90f);", 
                          "style.Colors[ImGuiCol_PopupBg]     = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);")

with open("src/Graphics/Renderer.cpp", "w") as f:
    f.write(content)
