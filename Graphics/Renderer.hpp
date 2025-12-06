#pragma once
#include "../include/glad/glad.h"
#include <SFML/Window.hpp>
#include "Camera.hpp"
#include "ShaderManager.hpp"
#include "Materials/PBRMaterial.hpp"
#include "Light.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>

class TextureManager;
class Model;
class Scene;

class Renderer {
private:
    static Renderer* s_instance;
    bool m_initialized;
    
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<TextureManager> m_textureManager;
    std::unique_ptr<class DebugQuad> m_debugQuad;
    bool m_showDebugAlbedo = false; // Debug visualization disabled
    
    // Shadow mapping - completely revised for all light types
    std::unordered_map<Light*, unsigned int> m_shadowFBOs;        // FBO per light
    std::unordered_map<Light*, unsigned int> m_shadowMaps;        // 2D shadow maps for directional/spot lights
    std::unordered_map<Light*, unsigned int> m_shadowCubemaps;    // Cubemap shadow maps for point lights
    std::unordered_map<Light*, unsigned int> m_shadowRBOs;        // Renderbuffer (depth) per light for color cubemap rendering
    // Keep spot/area/other shadow maps at 1024, directional at 8k
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    const unsigned int DIR_SHADOW_SIZE = 8192; // high-res directional shadow map
    const unsigned int CUBEMAP_SIZE = 2048;
    // Maximum spot shadow maps supported (must match shader MAX_SPOT_SHADOWS)
    const int MAX_SPOT_SHADOWS = 8;
    
    // Shaders for different shadow types
    unsigned int m_shadowShader;
    unsigned int m_pointShadowShader;
    
    // Multiple lights support
    std::vector<std::unique_ptr<Light>> m_lights;
    
    // Light visualization
    unsigned int m_lightVAO, m_lightVBO;

    Scene* m_scene;
    // Cached single light-space matrix (computed per shadow pass) so shaders sample the same
    std::unordered_map<Light*, glm::mat4> m_lightSpaceMatrices;

public:
    static Renderer& getInstance() {
        static Renderer instance;
        return instance;
    }
    
    Renderer();
    ~Renderer();
    
    bool initialize();
    void shutdown();
    
    void beginFrame();
    void endFrame();
    void renderScene(const Camera& camera);
    
    // Light management
    DirectionalLight* createDirectionalLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    PointLight* createPointLight(const glm::vec3& position, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f, float radius = 10.0f);
    SpotLight* createSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    // Area lights not supported in this build
    
    void removeLight(Light* light);
    void clearLights();
    const std::vector<std::unique_ptr<Light>>& getLights() const { return m_lights; }
    
    void setScene(Scene* scene);
    TextureManager& getTextureManager() { return *m_textureManager; }
    
private:
    void setupShadowMapping();
    void setupShaders();
    void setupLightVisualization();
    void renderShadowMaps();
    void renderDirectionalShadow(Light* light);
    void renderSpotShadow(Light* light);
    void renderPointShadow(Light* light);
    void setupLightUniforms(unsigned int shaderProgram, const Camera& camera);
    void renderLightSources(const Camera& camera);
    void renderLightVisualization(const Camera& camera, Light* light);
    void cleanupLightShadow(Light* light);
    void clearScreen();
};