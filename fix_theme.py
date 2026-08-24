
import re

with open("src/Graphics/Renderer.cpp", "r") as f:
    content = f.read()

new_style = """        // SpaceEngine inspired ImGui theme (Acrylic)
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;

        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;

        style.WindowPadding = ImVec2(8, 8);
        style.FramePadding = ImVec2(6, 4);
        style.ItemSpacing = ImVec2(6, 4);
        style.ItemInnerSpacing = ImVec2(4, 4);
        style.ScrollbarSize = 10.0f;
        style.GrabMinSize = 8.0f;

        // Acrylic transparent backgrounds (letting DrawAcrylicBackground shine through)
        style.Colors[ImGuiCol_WindowBg]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_ChildBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        style.Colors[ImGuiCol_Border]      = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        style.Colors[ImGuiCol_BorderShadow]= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        style.Colors[ImGuiCol_TitleBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
        style.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
        style.Colors[ImGuiCol_TitleBgCollapsed]= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        style.Colors[ImGuiCol_Text]         = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.15f, 0.15f, 0.15f, 0.55f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.75f);
        style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.35f, 0.35f, 0.35f, 0.95f);

        style.Colors[ImGuiCol_Button]        = ImVec4(0.15f, 0.15f, 0.15f, 0.55f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.75f);
        style.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.35f, 0.35f, 0.35f, 0.95f);

        style.Colors[ImGuiCol_CheckMark]      = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab]     = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

        style.Colors[ImGuiCol_Header]        = ImVec4(0.20f, 0.20f, 0.20f, 0.55f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.75f);
        style.Colors[ImGuiCol_HeaderActive]  = ImVec4(0.40f, 0.40f, 0.40f, 0.95f);

        style.Colors[ImGuiCol_Separator]        = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.60f, 0.60f);
        style.Colors[ImGuiCol_SeparatorActive]  = ImVec4(0.80f, 0.80f, 0.80f, 0.80f);

        style.Colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.90f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        style.Colors[ImGuiCol_TableHeaderBg]        = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
        style.Colors[ImGuiCol_TableBorderStrong]    = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
        style.Colors[ImGuiCol_TableBorderLight]     = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
        style.Colors[ImGuiCol_TableRowBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_TableRowBgAlt]        = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);

        style.Colors[ImGuiCol_Tab]           = ImVec4(0.10f, 0.10f, 0.10f, 0.80f);
        style.Colors[ImGuiCol_TabHovered]    = ImVec4(0.20f, 0.20f, 0.20f, 0.90f);
        style.Colors[ImGuiCol_TabSelected]   = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocused]  = ImVec4(0.05f, 0.05f, 0.05f, 0.80f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
"""

pattern = r"        // EVE Online inspired ImGui theme.*?style\.Colors\[ImGuiCol_TabUnfocused\]\s*=\s*ImVec4[^;]*;"
content = re.sub(pattern, new_style, content, flags=re.DOTALL)

with open("src/Graphics/Renderer.cpp", "w") as f:
    f.write(content)
print("Updated Renderer.cpp")
