#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos; // Passing world coordinates to the fragment shader
out vec3 Normal; // Passing normal to the fragment shader
out vec3 LocalPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    LocalPos = aPos;
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Normal matrix
    // Avoid distortion when scaling
    Normal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
