
import re

with open("src/main.cpp", "r") as f:
    content = f.read()

# Fix AddCircle
content = content.replace("drawList->AddCircle(screenPos, 4.0f,", "drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), 4.0f,")
content = content.replace("drawList->AddCircle(screenPos, 12.0f,", "drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), 12.0f,")

# Remove duplicate RIGHT PANEL
# The duplicate starts around line 500 with "// RIGHT PANEL (Celestial Bodies)" and ends right before "renderer.endUI();"
content = re.sub(r"        // RIGHT PANEL \(Celestial Bodies\).*?renderer\.endUI\(\);", "        renderer.endUI();", content, flags=re.DOTALL)

with open("src/main.cpp", "w") as f:
    f.write(content)
