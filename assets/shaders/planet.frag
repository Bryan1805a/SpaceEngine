#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 LocalPos; // Use LocalPos so the surface pattern rotates with the planet
in vec2 TexCoords;

// Declare 4 Textures
uniform sampler2D albedoMap;
uniform sampler2D specularMap;
uniform samplerCube depthMap;
uniform sampler2D emissionMap;

uniform vec3 lightPos;
uniform vec3 viewPos; // Camera position for calculating the atmospheric viewing angle
uniform float far_plane;
uniform int bodyType;
uniform float temperature;

// Check shadow mapping
float ShadowCalculation(vec3 FragPos) {
    vec3 fragToLight = FragPos - lightPos;
    float currentDepth = length(fragToLight);

    float bias = 0.15;
    float closestDepth = texture(depthMap, fragToLight).r * far_plane;

    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

// 3D Noise
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)), f.x),
                   mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
                   mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}
// Fractal Brownian Motion (Create undulating terrain)
float fbm(vec3 x) {
    float v = 0.0; float a = 0.5; vec3 shift = vec3(100.0);
    for (int i = 0; i < 4; ++i) {
        v += a * noise(x);
        x = x * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main() {
    // Calculate lightning
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    float shadow = ShadowCalculation(FragPos);

    // Solar luminous intensity
    float diff = max(dot(norm, lightDir), 0.0);
    float lightIntensity = diff * (1.0 - shadow);

    vec3 resultColor = vec3(0.0);
    vec3 bloomColor = vec3(0.0);
    vec3 emissionGlow = vec3(0.0); // Store self-illumination (like lava)

    // EARTH (Use Texture)
    if (bodyType == 1) {
        // Read colors from an image
        vec3 albedo = texture(albedoMap, TexCoords).rgb;
        float specMask = texture(specularMap, TexCoords).r;
        vec3 nightLights = texture(emissionMap, TexCoords).rgb;

        // Ambient light
        vec3 ambient = albedo * 0.02;

        // Diffused light (sunlight)
        vec3 diffuse = albedo * lightIntensity;

        // Sea surface sparkle (Specular)
        vec3 reflecDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflecDir), 0.0), 32.0);
        // // specMask will eliminate the sparkle effect if it is land (black)
        vec3 specular = vec3(1.0) * spec * specMask * lightIntensity;

        // City Light (Only lights up when plunged into darkness)
        float blendNight = smoothstep(0.1, 0.0, lightIntensity);
        vec3 cityGlow = nightLights * blendNight * vec3(1.0, 0.8, 0.5);

        // Color Synthesis
        resultColor = ambient + diffuse + specular + cityGlow;
        bloomColor = cityGlow;
    }
    // SUN or any other planets
    else if (bodyType == 0) {
        resultColor = vec3(10.0, 9.0, 7.0); // Warm white (HDR)
        bloomColor = resultColor;
    }
    else {
        vec3 baseCol = vec3(0.5);
        resultColor = baseCol * (0.1 + lightIntensity);
    }

    FragColor = vec4(resultColor, 1.0);
    BrightColor = vec4(bloomColor, 1.0);
}
