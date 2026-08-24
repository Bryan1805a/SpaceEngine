#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform vec4 orbitColor; // Màu sắc kèm độ trong suốt (Alpha)

void main() {
    FragColor = orbitColor;
    BrightColor = vec4(0.0); // Không làm lóa vệt quỹ đạo
}