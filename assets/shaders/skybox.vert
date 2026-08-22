#version 330 core
layout (location = 0) in vec2 aPos;

out vec3 viewRay; // Tia nhìn từ Camera

uniform mat4 invProjection;
uniform mat4 invView;

void main() {
    // Ép Z = 0.9999 để nó luôn nằm ở vô cực (xa nhất có thể)
    vec4 clipPos = vec4(aPos, 0.9999, 1.0);

    // Dùng ma trận nghịch đảo để lùi từ Màn hình 2D về Không gian 3D
    vec4 viewPos = invProjection * clipPos;
    viewRay = (invView * vec4(viewPos.xyz, 0.0)).xyz; // Lấy hướng tia nhìn

    gl_Position = clipPos;
}
