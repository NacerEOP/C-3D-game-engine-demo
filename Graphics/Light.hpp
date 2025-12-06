#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT,
    AREA
};

class Light {
public:
    virtual ~Light() = default;
    
    LightType type;
    glm::vec3 color;
    float intensity;
    bool castShadows;
    
    // ADDED: Shadow map handles
    unsigned int shadowMap = 0;
    unsigned int shadowCubemap = 0;
    
    virtual glm::mat4 getLightSpaceMatrix() const = 0;
    virtual void updateLightSpaceMatrix(const glm::vec3& sceneCenter) = 0;
    
    // ADDED: Get position for all lights
    virtual glm::vec3 getPosition() const = 0;
    
    // Getters
    LightType getType() const { return type; }
    glm::vec3 getColor() const { return color; }
    float getIntensity() const { return intensity; }
    bool getCastShadows() const { return castShadows; }
    
    // Setters
    void setColor(const glm::vec3& newColor) { color = newColor; }
    void setIntensity(float newIntensity) { intensity = newIntensity; }
    void setCastShadows(bool shadows) { castShadows = shadows; }
};

class DirectionalLight : public Light {
public:
    glm::vec3 direction;
    glm::vec3 position; // For shadow calculation
    
    DirectionalLight(const glm::vec3& dir, const glm::vec3& col = glm::vec3(1.0f), float intens = 1.0f);
    
    glm::mat4 getLightSpaceMatrix() const override;
    void updateLightSpaceMatrix(const glm::vec3& sceneCenter) override;
    
    void setDirection(const glm::vec3& dir);
    
    // ADDED: Get position implementation
    glm::vec3 getPosition() const override { return position; }
};

class PointLight : public Light {
public:
    glm::vec3 position;
    float radius;
    float constant;
    float linear;
    float quadratic;
    
    // ADDED: Shadow transforms for point light shadows
    std::vector<glm::mat4> shadowTransforms;
    
    PointLight(const glm::vec3& pos, const glm::vec3& col = glm::vec3(1.0f), float intens = 1.0f, float rad = 10.0f);
    
    glm::mat4 getLightSpaceMatrix() const override { return glm::mat4(1.0f); } // Not used for point lights
    void updateLightSpaceMatrix(const glm::vec3& sceneCenter) override {} // Not used for point lights
    
    void setPosition(const glm::vec3& pos);
    void setRadius(float rad);
    
    // ADDED: Shadow transform methods
    void setupShadowTransforms();
    const std::vector<glm::mat4>& getShadowTransforms() const { return shadowTransforms; }
    
    // ADDED: Get position implementation
    glm::vec3 getPosition() const override { return position; }
};

class SpotLight : public Light {
public:
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    float radius;
    
    SpotLight(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& col = glm::vec3(1.0f), 
              float intens = 1.0f, float innerAngle = 12.5f, float outerAngle = 17.5f);
    
    glm::mat4 getLightSpaceMatrix() const override;
    void updateLightSpaceMatrix(const glm::vec3& sceneCenter) override;
    
    void setPosition(const glm::vec3& pos);
    void setDirection(const glm::vec3& dir);
    void setAngles(float innerAngle, float outerAngle);
    
    // ADDED: Get position implementation
    glm::vec3 getPosition() const override { return position; }
};
// Area lights are not currently supported in this build