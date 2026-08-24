
import re

# Fix Renderer.hpp
with open("include/Graphics/Renderer.hpp", "r") as f:
    content = f.read()

# It was duplicated at line 159 (bottom of the class). I will just remove the second occurrence.
# Actually I added it to line 147 earlier.
lines = content.split("\n")
new_lines = []
for line in lines:
    if "GLFWwindow* getWindow() const {return window;}" in line:
        if "GLFWwindow" not in "".join(new_lines):
            new_lines.append(line)
    else:
        new_lines.append(line)

with open("include/Graphics/Renderer.hpp", "w") as f:
    f.write("\n".join(new_lines))


# Check main.cpp for missing braces
with open("src/main.cpp", "r") as f:
    content = f.read()

# Let"s just do a brace count check to see what"s wrong
open_braces = content.count("{")
close_braces = content.count("}")
print(f"Braces main.cpp: {{={open_braces}, }}={close_braces}")
