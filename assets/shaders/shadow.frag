#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main() {
    float lightDistance = length(FragPos.xyz - lightPos);
    // Normalise to [0.0, 1.0]
    lightDistance = lightDistance / far_plane;
    // Map to depth
    gl_FragDepth = lightDistance;
}