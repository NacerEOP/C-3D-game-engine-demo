#include "Light.hpp"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include <cmath>

// Directional Light
DirectionalLight::DirectionalLight(const glm::vec3& dir, const glm::vec3& col, float intens)
    : direction(glm::normalize(dir)), position(-dir * 20.0f) {
    type = LightType::DIRECTIONAL;
    color = col;
    intensity = intens;
    castShadows = true;
}

glm::mat4 DirectionalLight::getLightSpaceMatrix() const {
    float near_plane = 1.0f, far_plane = 50.0f;
    glm::mat4 lightProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}

void DirectionalLight::updateLightSpaceMatrix(const glm::vec3& sceneCenter) {
    position = sceneCenter - direction * 25.0f;
}

void DirectionalLight::setDirection(const glm::vec3& dir) {
    direction = glm::normalize(dir);
}

// Point Light
PointLight::PointLight(const glm::vec3& pos, const glm::vec3& col, float intens, float rad)
    : position(pos), radius(rad) {
    type = LightType::POINT;
    color = col;
    intensity = intens;
    castShadows = true; // Point light shadows are complex - disable by default
    constant = 1.0f;
    linear = 0.09f;
    quadratic = 0.032f;
}

// ADDED: Point light shadow transforms setup
void PointLight::setupShadowTransforms() {
    shadowTransforms.clear();
    
    float near_plane = 0.1f;
    float far_plane = radius;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
    
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
}

void PointLight::setPosition(const glm::vec3& pos) {
    position = pos;
}

void PointLight::setRadius(float rad) {
    radius = rad;
}

// Spot Light
SpotLight::SpotLight(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& col, 
                     float intens, float innerAngle, float outerAngle)
    : position(pos), direction(glm::normalize(dir)) {
    type = LightType::SPOT;
    color = col;
    intensity = intens;
    castShadows = true;
    cutOff = std::cos(glm::radians(innerAngle));
    outerCutOff = std::cos(glm::radians(outerAngle));
    radius = 15.0f;
}

glm::mat4 SpotLight::getLightSpaceMatrix() const {
    float near_plane = 1.0f;
    float far_plane = radius; // use configured radius for projection
    float fov = glm::acos(glm::clamp(outerCutOff, -0.9999f, 0.9999f)) * 2.0f; // radians
    glm::mat4 lightProjection = glm::perspective(fov, 1.0f, near_plane, far_plane);
    // Choose a stable up vector: if direction is nearly parallel to world up, use an alternate up
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 up = (std::abs(glm::dot(direction, worldUp)) > 0.99f) ? glm::vec3(1.0f, 0.0f, 0.0f) : worldUp;
    glm::mat4 lightView = glm::lookAt(position, position + direction, up);
    return lightProjection * lightView;
}

void SpotLight::updateLightSpaceMatrix(const glm::vec3& sceneCenter) {
    // Spot light doesn't need scene center for matrix calculation
}

void SpotLight::setPosition(const glm::vec3& pos) {
    position = pos;
}

void SpotLight::setDirection(const glm::vec3& dir) {
    direction = glm::normalize(dir);
}

void SpotLight::setAngles(float innerAngle, float outerAngle) {
    cutOff = std::cos(glm::radians(innerAngle));
    outerCutOff = std::cos(glm::radians(outerAngle));
}

// Area Light
// AreaLight implementation removed. Area lights not supported in this build.