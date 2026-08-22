#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 LocalPos; // Use LocalPos so the surface pattern rotates with the planet

uniform vec3 lightPos;
uniform vec3 viewPos; // Camera position for calculating the atmospheric viewing angle

uniform int bodyType;
uniform float temperature;

uniform samplerCube depthMap;
uniform float far_plane;

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
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient = vec3(0.01);

    vec3 resultColor = vec3(0.0);
    vec3 emissionGlow = vec3(0.0); // Store self-illumination (like lava)

    float shadow = ShadowCalculation(FragPos);

    // SUN
    if (bodyType == 0) {
        resultColor = vec3(3.0, 2.8, 2.5); // Warm white (HDR)
        emissionGlow = resultColor;
    }
    // ROCKY PLANETS (Temperature Interaction)
    else if (bodyType == 1) {
        float elevation = fbm(LocalPos * 5.0); // Generate continental elevation

        if (temperature > 800.0) {
            // Scenario A: Lava Continent
            // Caused by drifting close to the Sun
            vec3 crust = vec3(0.1, 0.05, 0.05); // Volcanic rock
            vec3 lava = vec3(3.0, 0.8, 0.0);    // Blazing lava
            float lavaMask = smoothstep(0.4, 0.6, elevation);
            resultColor = mix(lava, crust, lavaMask) * (ambient + diff);

            if (lavaMask < 0.5) emissionGlow = lava * (1.0 - lavaMask);
        }
        else if (temperature > 250.0 && temperature < 350.0) {
            // Scenario B: Earth (With life)
            vec3 water = vec3(0.02, 0.15, 0.4);
            vec3 land = mix(vec3(0.1, 0.4, 0.1), vec3(0.5, 0.4, 0.2), fbm(LocalPos * 10.0));

            // Two freezing poles based on the Y-axis and temperature
            float poleMask = smoothstep(0.6, 0.9, abs(LocalPos.y)) * smoothstep(350.0, 250.0, temperature);
            vec3 ice = vec3(0.9, 0.9, 1.0);

            vec3 baseCol = (elevation < 0.5) ? water : land;
            baseCol = mix(baseCol, ice, poleMask); // Snow-capped peaks

            resultColor = baseCol * (ambient + (1.0 - shadow) * diff);

            // Atmospheric Scattering (Fresnel Effect)
            // Create a blue halo only at the planet's edge and on the illuminated side
            float fresnel = pow(1.0 - max(dot(norm, viewDir), 0.0), 3.0);
            vec3 atmosphere = vec3(0.2, 0.5, 1.0) * fresnel * diff;
            resultColor += atmosphere;
        }
        else {
            // Scenario C: Frozen Planet / Deadly (Mars / Pluto)
            vec3 barren = mix(vec3(0.5, 0.3, 0.2), vec3(0.8, 0.9, 1.0), smoothstep(0.0, 150.0, 250.0 - temperature));
            resultColor = barren * elevation * (ambient + (1.0 - shadow) * diff);
        }
    }
    // 3. GAS PLANET (Jupiter)
    else if (bodyType == 2) {
        // Cloud bands running across the Y axis
        float bands = fbm(vec3(LocalPos.y * 15.0, LocalPos.x, LocalPos.z));
        vec3 gas1 = vec3(0.7, 0.6, 0.5);
        vec3 gas2 = vec3(0.5, 0.3, 0.1);
        resultColor = mix(gas1, gas2, bands) * (ambient + diff);
    }
    // 4. ICE MOON
    else if (bodyType == 3) {
        float craters = fbm(LocalPos * 6.0);
        vec3 ice = mix(vec3(0.5, 0.6, 0.7), vec3(0.9, 0.95, 1.0), craters);
        resultColor = ice * (ambient + diff);
    }
    // 5. ASTEROID
    else if (bodyType == 4) {
        float rough = fbm(LocalPos * 8.0);
        vec3 rock = mix(vec3(0.2, 0.18, 0.15), vec3(0.45, 0.42, 0.38), rough);
        resultColor = rock * (ambient + diff);
    }

    FragColor = vec4(resultColor, 1.0);

    // If the overall brightness or lava intensity is high enough, switch to Texture Bloom
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0 || length(emissionGlow) > 0.0)
        BrightColor = vec4(max(FragColor.rgb, emissionGlow), 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
