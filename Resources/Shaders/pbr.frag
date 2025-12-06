#version 450 core
out vec4 FragColor;

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
    vec2 TexCoords1;
    vec4 FragPosLightSpace;
} fs_in;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;
uniform vec3 emission;
uniform float emissionStrength;

// Texture maps (optional)
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D aoMap;
uniform sampler2D emissionMap;

uniform int useAlbedoMap;
uniform int useNormalMap;
uniform int useMetallicRoughnessMap;
uniform int useAOMap;
uniform int useEmissionMap;

// UV transforms (glTF KHR_texture_transform support)
uniform vec2 albedoUVScale = vec2(1.0);
uniform vec2 albedoUVOffset = vec2(0.0);
uniform vec2 normalUVScale = vec2(1.0);
uniform vec2 normalUVOffset = vec2(0.0);
uniform vec2 metallicRoughnessUVScale = vec2(1.0);
uniform vec2 metallicRoughnessUVOffset = vec2(0.0);
uniform vec2 aoUVScale = vec2(1.0);
uniform vec2 aoUVOffset = vec2(0.0);
uniform vec2 emissionUVScale = vec2(1.0);
uniform vec2 emissionUVOffset = vec2(0.0);
// Which TEXCOORD set to use for each map (0 or 1)
uniform int albedoUVSet = 0;
uniform int normalUVSet = 0;
uniform int metallicRoughnessUVSet = 0;
uniform int aoUVSet = 0;
uniform int emissionUVSet = 0;

// Camera
uniform vec3 viewPos;

// Light structures - ADD castShadows BOOL
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
    bool castShadows;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
    float constant;
    float linear;
    float quadratic;
    bool castShadows;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
    bool castShadows;
};

// Light arrays
uniform DirectionalLight directionalLights[4];
uniform PointLight pointLights[16];
uniform SpotLight spotLights[8];

uniform int directionalLightCount;
uniform int pointLightCount;
uniform int spotLightCount;

// Shadow mapping
// Directional shadow map (single-map)
uniform sampler2D shadowMap;
// Individual spot shadow maps
#define MAX_SPOT_SHADOWS 8
uniform sampler2D spotShadowMaps[MAX_SPOT_SHADOWS];
uniform mat4 spotLightSpaceMatrices[MAX_SPOT_SHADOWS];
uniform samplerCube pointShadowMap;

uniform float pointFarPlane;

uniform mat4 lightSpaceMatrix;

const float PI = 3.14159265359;

// Shadow calculation for directional lights (single shadow map)
float directionalShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if (projCoords.z > 1.0) return 0.0;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    // Use a much smaller, slope-scaled bias to avoid peter-panning
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.0001);

    // 7x7 PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -3; x <= 3; ++x) {
        for(int y = -3; y <= 3; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 49.0;

    return shadow;
}

// Shadow calculation for spot lights using if/else to avoid dynamic indexing
float spotShadowCalculation(int index, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(projCoords.z > 1.0) return 0.0;

    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.00001);
    float shadow = 0.0;
    vec2 texelSize;

    // Use if/else to sample correct spotlight shadow map (5x5 PCF)
    if(index == 0) {
        texelSize = 1.0 / textureSize(spotShadowMaps[0], 0);
        for(int x = -2; x <= 2; ++x) {
            for(int y = -2; y <= 2; ++y) {
                float pcfDepth = texture(spotShadowMaps[0], projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
    } else if(index == 1) {
        texelSize = 1.0 / textureSize(spotShadowMaps[1], 0);
        for(int x = -2; x <= 2; ++x) {
            for(int y = -2; y <= 2; ++y) {
                float pcfDepth = texture(spotShadowMaps[1], projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
    } else if(index == 2) {
        texelSize = 1.0 / textureSize(spotShadowMaps[2], 0);
        for(int x = -2; x <= 2; ++x) {
            for(int y = -2; y <= 2; ++y) {
                float pcfDepth = texture(spotShadowMaps[2], projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
    } else if(index == 3) {
        texelSize = 1.0 / textureSize(spotShadowMaps[3], 0);
        for(int x = -2; x <= 2; ++x) {
            for(int y = -2; y <= 2; ++y) {
                float pcfDepth = texture(spotShadowMaps[3], projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
    } else if(index == 4) {
        texelSize = 1.0 / textureSize(spotShadowMaps[4], 0);
        for(int x = -2; x <= 2; ++x) {
            for(int y = -2; y <= 2; ++y) {
                float pcfDepth = texture(spotShadowMaps[4], projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
    } else if(index == 5) {
        texelSize = 1.0 / textureSize(spotShadowMaps[5], 0);
        for(int x = -2; x <= 2; ++x) {
            for(int y = -2; y <= 2; ++y) {
                float pcfDepth = texture(spotShadowMaps[5], projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
    } else {
        return 0.0;
    }
    
    shadow /= 25.0;
    return shadow;
}

// Shadow calculation for area lights (approximate using a single shadow map)
// Area lights not supported in this build

// PBR Functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Point light attenuation with smooth falloff
float calculateAttenuation(float distance, float radius) {
    float attenuation = 1.0 / (distance * distance);
    float smoothFactor = 1.0 - smoothstep(0.0, radius, distance);
    return attenuation * smoothFactor;
}

void main() {
    // Choose UV set per texture (respect glTF texCoord) then apply glTF texture transforms
    vec2 baseUV0 = fs_in.TexCoords;
    vec2 baseUV1 = fs_in.TexCoords1;

    vec2 baseUV;
    // Albedo
    baseUV = (albedoUVSet == 1) ? baseUV1 : baseUV0;
    vec2 albedoUV = baseUV * albedoUVScale + albedoUVOffset;
    // Normal
    baseUV = (normalUVSet == 1) ? baseUV1 : baseUV0;
    vec2 normalUV = baseUV * normalUVScale + normalUVOffset;
    // MetallicRoughness
    baseUV = (metallicRoughnessUVSet == 1) ? baseUV1 : baseUV0;
    vec2 metallicRoughnessUV = baseUV * metallicRoughnessUVScale + metallicRoughnessUVOffset;
    // AO
    baseUV = (aoUVSet == 1) ? baseUV1 : baseUV0;
    vec2 aoUV = baseUV * aoUVScale + aoUVOffset;
    // Emission
    baseUV = (emissionUVSet == 1) ? baseUV1 : baseUV0;
    vec2 emissionUV = baseUV * emissionUVScale + emissionUVOffset;
    
    // Normal calculation
    // Sample material textures (if present) and derive final material parameters
    vec3 baseColor = albedo;
    if (useAlbedoMap == 1) {
        // Use the texture directly - don't multiply by uniform albedo (which is a fallback dark value)
        baseColor = texture(albedoMap, albedoUV).rgb;
    }

    // Apply normal mapping if available
    vec3 N = normalize(fs_in.Normal);
    if (useNormalMap == 1) {
        vec3 normal = texture(normalMap, normalUV).rgb;
        normal = normalize(normal * 2.0 - 1.0); // Convert from [0,1] to [-1,1]
        // Reconstruct N using normal map in tangent space
        vec3 T = normalize(fs_in.WorldPos - dot(fs_in.WorldPos, N) * N); // Approximate tangent
        vec3 B = cross(N, T);
        mat3 TBN = mat3(normalize(T), normalize(B), N);
        N = normalize(TBN * normal);
    } else {
        vec3 geometricNormal = normalize(cross(dFdx(fs_in.WorldPos), dFdy(fs_in.WorldPos)));
        vec3 interpNormal = normalize(fs_in.Normal);
        N = -normalize(faceforward(geometricNormal, interpNormal, interpNormal));
    }

    float metallicFactor = metallic;
    float roughnessFactor = roughness;
    if (useMetallicRoughnessMap == 1) {
        vec4 mr = texture(metallicRoughnessMap, metallicRoughnessUV);
        // Common glTF packing: G = roughness, B = metallic
        roughnessFactor *= mr.g;
        metallicFactor *= mr.b;
    }

    float aoFactor = ao;
    if (useAOMap == 1) {
        aoFactor *= texture(aoMap, aoUV).r;
    }

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallicFactor);

    // Reflectance equation
    vec3 Lo = vec3(0.0);
    vec3 V = normalize(viewPos - fs_in.WorldPos);

    // Directional lights - WITH SHADOWS
    for(int i = 0; i < directionalLightCount; i++) {
        DirectionalLight light = directionalLights[i];
        
        vec3 L = normalize(-light.direction);
        vec3 H = normalize(V + L);
        
        // Bright lighting
        vec3 radiance = light.color * light.intensity * 10.0;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughnessFactor);   
        float G = GeometrySmith(N, V, L, roughnessFactor);    
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallicFactor; 	
            
        float NdotL = max(dot(N, L), 0.0);                

        // CRITICAL FIX: Only calculate shadow if light.castShadows is true
        float shadow = 0.0;
        if(light.castShadows) {
            shadow = directionalShadowCalculation(fs_in.FragPosLightSpace, N, L);
        }

        // Apply shadow to direct lighting (use sampled baseColor when available)
        vec3 directLighting = (kD * baseColor / PI + specular) * radiance * NdotL;
        Lo += (1.0 - shadow) * directLighting;
    }

    // Point lights - NO SHADOWS
    for(int i = 0; i < pointLightCount; i++) {
        PointLight light = pointLights[i];
        
        vec3 L = normalize(light.position - fs_in.WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(light.position - fs_in.WorldPos);
        
        // Skip if beyond radius
        if(distance > light.radius) continue;
        
        // Smooth attenuation
        float attenuation = calculateAttenuation(distance, light.radius);
        vec3 radiance = light.color * light.intensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughnessFactor);   
        float G = GeometrySmith(N, V, L, roughnessFactor);    
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallicFactor; 	
            
        float NdotL = max(dot(N, L), 0.0);                

        // Shadow calculation for point lights (using cubemap distance map)
        float shadow = 0.0;
        if (light.castShadows) {
            vec3 fragToLight = fs_in.WorldPos - light.position;
            float currentDepth = length(fragToLight);
            float closestDepth = texture(pointShadowMap, fragToLight).r; // stored distance in cubemap
            float bias = 0.005; // reduced bias for point lights
            shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }

        vec3 directLighting = (kD * baseColor / PI + specular) * radiance * NdotL;
        Lo += (1.0 - shadow) * directLighting;
    }

    // Spot lights - WITH SHADOWS (DISABLED WHILE DEBUGGING)
    for(int i = 0; i < spotLightCount; i++) {
        SpotLight light = spotLights[i];
        
        vec3 L = normalize(light.position - fs_in.WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(light.position - fs_in.WorldPos);
        
        // Spot light cone calculation
        float theta = dot(L, normalize(-light.direction));
        float epsilon = light.cutOff - light.outerCutOff;
        float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
        
        if(intensity <= 0.0) continue;
        
        // Smooth attenuation
        float attenuation = calculateAttenuation(distance, 25.0);
        vec3 radiance = light.color * light.intensity * attenuation * intensity;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughnessFactor);   
        float G = GeometrySmith(N, V, L, roughnessFactor);    
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallicFactor; 	
            
        float NdotL = max(dot(N, L), 0.0);                

        // Calculate spot shadow for this light
        float shadow = 0.0;
        if(light.castShadows) {
            vec4 spotFragPosLightSpace = spotLightSpaceMatrices[i] * vec4(fs_in.WorldPos, 1.0);
            shadow = spotShadowCalculation(i, spotFragPosLightSpace, N, L);
        }

        vec3 directLighting = (kD * baseColor / PI + specular) * radiance * NdotL;
        Lo += (1.0 - shadow) * directLighting;
    }

    // Area lights not supported

    // Ambient lighting
    vec3 ambient = vec3(0.03) * baseColor * aoFactor;

    vec3 color = ambient + Lo;

    // Emission
    vec3 emissionColor = emission;
    if(useEmissionMap != 0) {
        emissionColor = texture(emissionMap, emissionUV).rgb;
    }
    
    // If emission strength is significant, make it unlit (pure emission material)
    if(emissionStrength > 0.5) {
        // Pure emission: no lighting influence, skip tonemapping
        color = emissionColor;
        FragColor = vec4(color, 1.0);
        return;
    } else {
        color += emissionColor * emissionStrength;
    }

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // NOTE: Gamma correction is now handled by GL_FRAMEBUFFER_SRGB on the framebuffer
    // Do NOT apply manual gamma correction here

    FragColor = vec4(color, 1.0);
}

// Area light shadow helper removed