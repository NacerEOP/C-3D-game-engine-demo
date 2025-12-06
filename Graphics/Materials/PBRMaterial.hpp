#pragma once
#include <glm/glm.hpp>
#include <string>

struct PBRMaterial {
    glm::vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    glm::vec3 emission;
    float emissionStrength;
    
    // Texture paths (optional)
    std::string albedoMap;
    std::string normalMap;
    std::string metallicMap;
    std::string roughnessMap;
    std::string aoMap;
    std::string emissionMap;
    
    PBRMaterial(const glm::vec3& albedo = glm::vec3(0.9f), 
                float metallic = 0.1f, 
                float roughness = 0.4f, 
                float ao = 1.0f,
                const glm::vec3& emission = glm::vec3(0.0f),
                float emissionStrength = 0.0f)
        : albedo(albedo), metallic(metallic), roughness(roughness), ao(ao),
          emission(emission), emissionStrength(emissionStrength) {}
};