#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor; // FBO requires both color attachments

in vec3 viewRay;

uniform sampler2D skybox; // Equirectangular HDR environment map

const float PI = 3.14159265359;

void main() {
    vec3 dir = normalize(viewRay);

    // Convert the world-space ray direction into equirectangular UV coordinates.
    //   u: azimuth (0..1 around the horizon), v: elevation (-90..90 degrees)
    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    float v = asin(dir.y) / PI + 0.5;

    vec3 color = texture(skybox, vec2(u, v)).rgb;

    FragColor = vec4(color, 1.0);
    // Keep the environment out of the bloom pass so it stays calm and cold
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
