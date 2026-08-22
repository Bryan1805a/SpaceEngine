#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture; // Scene
uniform sampler2D bloomBlur;     // The light has been blurred

void main() {
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    // Additive Blending
    hdrColor += bloomColor;

    // Tone Mapping
    // Compressing HDR lighting back into the display's LDR color space
    // The Exposure Tone Mapping algorithm creates a glare effect similar to that perceived by the human eye
    float exposure = 1.0;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

    // Vignette effect
    vec2 center = TexCoords - vec2(0.5);
    float dist = length(center);
    float vignette = smoothstep(0.8, 0.2, dist);
    mapped *= vignette;

    FragColor = vec4(mapped, 1.0);
}
