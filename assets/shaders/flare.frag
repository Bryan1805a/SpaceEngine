#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

// Sun's screen position in UV space [0,1] and a flag that zeroes the flare when
// the Sun is behind a planet (occluded) or behind the camera.
uniform vec2 sunPos;
uniform float sunVisible;  // 1.0 = Sun visible, 0.0 = hidden
uniform float aspect;      // width / height, to keep flare elements round
uniform float intensity;   // master brightness scale tied to the Sun's prominence
uniform float baseRadius;  // Sun's actual radius in UV space

// Soft radial falloff around an element centre.
float glow(float d, float radius) {
    if (d >= radius) return 0.0;
    float t = 1.0 - d / radius;
    return t * t * t; // ^3 skirt
}

// Thin bright ring (diffraction) around the source.
float ring(float d, float radius, float thickness) {
    float band = 1.0 - abs(d - radius) / thickness;
    return band > 0.0 ? band * band : 0.0;
}

void main() {
    if (sunVisible < 0.5) discard;

    vec2 uv = TexCoords;

    // Aspect-corrected offset from the Sun's position.
    vec2 dir = vec2((uv.x - sunPos.x) * aspect, uv.y - sunPos.y);
    float d0 = length(dir);

    vec3 col = vec3(0.0);

    // Primary halo hugging the Sun (3 to 5 times the size of the sun).
    col += vec3(1.00, 0.85, 0.60) * glow(d0, baseRadius * 5.0) * 0.28;
    col += vec3(1.00, 0.95, 0.80) * glow(d0, baseRadius * 3.0) * 0.75;
    col += vec3(0.65, 0.80, 1.00) * ring(d0, baseRadius * 4.0, baseRadius * 0.15) * 0.22;

    // Axis from the Sun toward the centre of the screen.
    vec2 axis = vec2((0.5 - sunPos.x) * aspect, 0.5 - sunPos.y);
    float axisLen = length(axis);

    // Ghost blobs scattered along that axis.
    if (axisLen > 1e-4) {
        axis /= axisLen;

        const int N = 6;
        float t[N] = float[N](-0.55, 0.30, 0.55, 0.85, 1.15, 1.50);
        float radius[N] = float[N](0.035, 0.045, 0.025, 0.070, 0.040, 0.030);
        vec3 tint[N] = vec3[N](
            vec3(0.80, 0.65, 0.35),
            vec3(1.00, 0.80, 0.45),
            vec3(0.55, 0.75, 1.00),
            vec3(0.95, 0.90, 0.70),
            vec3(0.80, 0.70, 0.50),
            vec3(0.60, 0.80, 1.00));

        for (int i = 0; i < N; ++i) {
            vec2 centre = sunPos + axis * t[i] * axisLen;
            vec2 off = vec2((uv.x - centre.x) * aspect, uv.y - centre.y);
            col += tint[i] * glow(length(off), radius[i]) * 0.45;
        }
    }

    // Horizontal anamorphic streak through the Sun.
    if (abs(dir.y) < 0.006 && d0 > 0.0) {
        col += vec3(1.00, 0.90, 0.70) * exp(-abs(dir.x) * 3.0) * 0.12;
    }

    FragColor = vec4(col * intensity, 1.0);
}
