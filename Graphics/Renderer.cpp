#include "Renderer.hpp"
#include "../include/glad/glad.h"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "../include/glm/gtc/type_ptr.hpp"
#include "../Utils/Logger.hpp"
#include "TextureManager.hpp"
#include "DebugQuad.hpp"
#include "../Scene/Scene.hpp"
#include <iostream>
#include <vector>

Renderer::Renderer() 
    : m_initialized(false)
    , m_shaderManager(std::make_unique<ShaderManager>())
    , m_textureManager(std::make_unique<TextureManager>())
    , m_shadowShader(0)
    , m_pointShadowShader(0)
    , m_lightVAO(0), m_lightVBO(0)
    , m_scene(nullptr) {
    Logger::info("Renderer constructed");
}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    if (m_initialized) return true;

    if (!gladLoadGL()) {
        Logger::error("Failed to initialize GLAD!");
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    // Enable seamless cubemap sampling for smoother point shadow lookups
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    // Enable sRGB framebuffer mode so hardware does final gamma correction
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.4f, 0.6f, 0.9f, 1.0f);
    
    Logger::info(std::string("PBR Renderer initialized: ") + (const char*)glGetString(GL_VERSION));

    // Log GL limits helpful for debugging texture unit exhaustion
    GLint maxTexUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);
    Logger::info(std::string("GL_MAX_TEXTURE_IMAGE_UNITS = ") + std::to_string(maxTexUnits));

    setupShaders();
    setupShadowMapping();
    setupLightVisualization();

    // Load debug shader for fullscreen albedo visualization
    if (!m_shaderManager->loadShaderFromFile("debug_quad", "Resources/Shaders/debug_quad.vert", "Resources/Shaders/debug_quad.frag")) {
        Logger::warn("Failed to load debug quad shader (optional)");
    }
    m_debugQuad = std::make_unique<DebugQuad>();

    // COMMENTED OUT: No default light - you create lights in your scene
    // createDirectionalLight(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f), 1.0f);

    m_initialized = true;
    return true;
}

// Light Management
DirectionalLight* Renderer::createDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) {
    auto light = std::make_unique<DirectionalLight>(direction, color, intensity);
    DirectionalLight* ptr = light.get();
    m_lights.push_back(std::move(light));
    Logger::info("Created directional light");
    return ptr;
}

PointLight* Renderer::createPointLight(const glm::vec3& position, const glm::vec3& color, float intensity, float radius) {
    auto light = std::make_unique<PointLight>(position, color, intensity, radius);
    PointLight* ptr = light.get();
    m_lights.push_back(std::move(light));
    Logger::info("Created point light");
    return ptr;
}

SpotLight* Renderer::createSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, float intensity) {
    auto light = std::make_unique<SpotLight>(position, direction, color, intensity, 12.5f, 17.5f);
    SpotLight* ptr = light.get();
    m_lights.push_back(std::move(light));
    Logger::info("Created spot light");
    return ptr;
}

// Area lights not supported in this build; creation removed.

void Renderer::removeLight(Light* light) {
    // Clean up shadow resources first
    cleanupLightShadow(light);
    
    // Remove from lights list
    m_lights.erase(std::remove_if(m_lights.begin(), m_lights.end(),
        [light](const std::unique_ptr<Light>& l) { return l.get() == light; }), m_lights.end());
}

void Renderer::clearLights() {
    // Clean up all shadow resources
    for (auto& light : m_lights) {
        cleanupLightShadow(light.get());
    }
    m_lights.clear();
}

void Renderer::setupShaders() {
    if (!m_shaderManager->loadShaderFromFile("pbr", "Resources/Shaders/pbr.vert", "Resources/Shaders/pbr.frag")) {
        Logger::error("Failed to load PBR shader!");
    }
    
    // Standard shadow shader for directional and spot lights
    if (!m_shaderManager->loadShaderFromFile("shadow", "Resources/Shaders/shadow.vert", "Resources/Shaders/shadow.frag")) {
        Logger::error("Failed to load shadow shader!");
    }
    
    // Point shadow shader for omnidirectional shadows
    if (!m_shaderManager->loadShaderFromFile("point_shadow", "Resources/Shaders/point_shadow.vert", "Resources/Shaders/point_shadow.frag")) {
        Logger::error("Failed to load point shadow shader!");
    }
    
    if (!m_shaderManager->loadShaderFromFile("light", "Resources/Shaders/light.vert", "Resources/Shaders/light.frag")) {
        Logger::error("Failed to load light shader!");
    }
    
    m_shadowShader = m_shaderManager->getShader("shadow");
    m_pointShadowShader = m_shaderManager->getShader("point_shadow");
}

void Renderer::setupShadowMapping() {
    // Shadow maps created on-demand per light
    Logger::info("Shadow mapping system initialized");
}

void Renderer::cleanupLightShadow(Light* light) {
    // Clean up shadow resources for a specific light
    if (m_shadowFBOs.find(light) != m_shadowFBOs.end()) {
        glDeleteFramebuffers(1, &m_shadowFBOs[light]);
        m_shadowFBOs.erase(light);
    }
    if (m_shadowMaps.find(light) != m_shadowMaps.end()) {
        glDeleteTextures(1, &m_shadowMaps[light]);
        m_shadowMaps.erase(light);
    }
    if (m_shadowCubemaps.find(light) != m_shadowCubemaps.end()) {
        glDeleteTextures(1, &m_shadowCubemaps[light]);
        m_shadowCubemaps.erase(light);
    }
    if (m_shadowRBOs.find(light) != m_shadowRBOs.end()) {
        glDeleteRenderbuffers(1, &m_shadowRBOs[light]);
        m_shadowRBOs.erase(light);
    }
}

void Renderer::setupLightVisualization() {
    // Cube vertex list laid out as triangles (6 faces x 2 triangles x 3 vertices = 36 vertices)
    float cubeVertices[] = {
        // Front face (+Z)
        -1, -1,  1,  1, -1,  1,  1,  1,  1,
         1,  1,  1, -1,  1,  1, -1, -1,  1,
        // Back face (-Z)
        -1, -1, -1, -1,  1, -1,  1,  1, -1,
         1,  1, -1,  1, -1, -1, -1, -1, -1,
        // Left face (-X)
        -1, -1, -1, -1, -1,  1, -1,  1,  1,
        -1,  1,  1, -1,  1, -1, -1, -1, -1,
        // Right face (+X)
         1, -1, -1,  1,  1, -1,  1,  1,  1,
         1,  1,  1,  1, -1,  1,  1, -1, -1,
        // Top face (+Y)
        -1,  1, -1, -1,  1,  1,  1,  1,  1,
         1,  1,  1,  1,  1, -1, -1,  1, -1,
        // Bottom face (-Y)
        -1, -1, -1,  1, -1, -1,  1, -1,  1,
         1, -1,  1, -1, -1,  1, -1, -1, -1
    };

    glGenVertexArrays(1, &m_lightVAO);
    glGenBuffers(1, &m_lightVBO);
    glBindVertexArray(m_lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void Renderer::renderShadowMaps() {
    // Render shadows for ALL lights that cast them
    for (auto& light : m_lights) {
        if (!light->castShadows) continue;
        switch (light->type) {
            case LightType::DIRECTIONAL:
                renderDirectionalShadow(light.get());
                break;
            case LightType::SPOT:
                renderSpotShadow(light.get());
                break;
            case LightType::POINT:
                renderPointShadow(light.get());
                break;
            // Area lights are not handled here
        }
    }
}

void Renderer::renderDirectionalShadow(Light* light) {
    DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);

    // Create shadow map if it doesn't exist
    if (m_shadowMaps.find(light) == m_shadowMaps.end()) {
        unsigned int shadowFBO, shadowMap;
        glGenFramebuffers(1, &shadowFBO);
        glGenTextures(1, &shadowMap);

        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, DIR_SHADOW_SIZE, DIR_SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::error("Directional shadow framebuffer not complete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_shadowFBOs[light] = shadowFBO;
        m_shadowMaps[light] = shadowMap;
        light->shadowMap = shadowMap;

        Logger::info("Created directional light shadow map");
    }

    // Compute a reasonable orthographic projection based on scene bounds
    glm::vec3 sceneCenter(0.0f);
    float sceneRadius = 25.0f;
    if (m_scene) m_scene->computeSceneBounds(sceneCenter, sceneRadius);

    glm::vec3 lightDir = glm::normalize(dirLight->direction);
    dirLight->position = sceneCenter - lightDir * (sceneRadius + 10.0f);

    float halfDim = std::max(25.0f, sceneRadius * 1.2f);
    float near_plane = 1.0f;
    float far_plane = sceneRadius + 60.0f;

    glm::mat4 lightProjection = glm::ortho(-halfDim, halfDim, -halfDim, halfDim, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(dirLight->position, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Cache the matrix for the main pass
    m_lightSpaceMatrices[light] = lightSpaceMatrix;

    // Render to the shadow map
    glViewport(0, 0, DIR_SHADOW_SIZE, DIR_SHADOW_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOs[light]);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glUseProgram(m_shadowShader);
    glUniformMatrix4fv(glGetUniformLocation(m_shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    if (m_scene) m_scene->renderForShadow(m_shadowShader);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderSpotShadow(Light* light) {
    SpotLight* spotLight = static_cast<SpotLight*>(light);
    Logger::info(std::string("renderSpotShadow called for spot at: ") +
                 std::to_string(spotLight->position.x) + "," + std::to_string(spotLight->position.y) + "," + std::to_string(spotLight->position.z));
    
    // Create shadow map if it doesn't exist
    if (m_shadowMaps.find(light) == m_shadowMaps.end()) {
        unsigned int shadowFBO, shadowMap;
        glGenFramebuffers(1, &shadowFBO);
        glGenTextures(1, &shadowMap);
        
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::error("Spot light shadow framebuffer not complete!");
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        m_shadowFBOs[light] = shadowFBO;
        m_shadowMaps[light] = shadowMap;
        light->shadowMap = shadowMap;
        
        Logger::info("Created spot light shadow map");
    }
    
    // Render shadow map
    Logger::info("Rendering spot shadow map for light");
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOs[light]);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    
    glUseProgram(m_shadowShader);
    
    glm::mat4 lightSpaceMatrix = spotLight->getLightSpaceMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    
    if (m_scene) {
        m_scene->renderForShadow(m_shadowShader);
    }
    Logger::info("Finished rendering spot shadow map for light");
    
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderPointShadow(Light* light) {
    PointLight* pointLight = static_cast<PointLight*>(light);
    
    // Create cubemap shadow if it doesn't exist
    if (m_shadowCubemaps.find(light) == m_shadowCubemaps.end()) {
        unsigned int shadowFBO, shadowCubemap;
        glGenFramebuffers(1, &shadowFBO);
        glGenTextures(1, &shadowCubemap);

        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap);
        // Create floating-point color cubemap to store linear distances
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F,
                         CUBEMAP_SIZE, CUBEMAP_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        // We'll attach each face as a color attachment when rendering per-face
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        GLenum drawBuf = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &drawBuf);

        // Create and attach a depth renderbuffer so depth testing works when rendering into the color cubemap
        unsigned int rboDepth;
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, CUBEMAP_SIZE, CUBEMAP_SIZE);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

        m_shadowRBOs[light] = rboDepth;

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::error("Point light shadow cubemap framebuffer not complete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_shadowFBOs[light] = shadowFBO;
        m_shadowCubemaps[light] = shadowCubemap;
        pointLight->shadowCubemap = shadowCubemap;

        // Setup shadow transforms for point light
        pointLight->setupShadowTransforms();

        Logger::info("Created point light shadow cubemap (color)");
    }
    
    // Render scene to each face of the cubemap (per-face render)
    glViewport(0, 0, CUBEMAP_SIZE, CUBEMAP_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOs[light]);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glUseProgram(m_pointShadowShader);

    // Set light properties used by fragment shader
    GLint lightPosLoc = glGetUniformLocation(m_pointShadowShader, "lightPos");
    GLint farPlaneLoc = glGetUniformLocation(m_pointShadowShader, "far_plane");
    glUniform3f(lightPosLoc, pointLight->position.x, pointLight->position.y, pointLight->position.z);
    glUniform1f(farPlaneLoc, pointLight->radius);

    auto& shadowTransforms = pointLight->getShadowTransforms();

    // Render each cubemap face separately, attaching the appropriate face as the framebuffer depth attachment
    for (unsigned int i = 0; i < 6; ++i) {
        // Attach the cubemap face as the color attachment
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pointLight->shadowCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set the per-face shadow matrix
        GLint shadowMatLoc = glGetUniformLocation(m_pointShadowShader, "shadowMatrix");
        glUniformMatrix4fv(shadowMatLoc, 1, GL_FALSE, &shadowTransforms[i][0][0]);

        if (m_scene) {
            m_scene->renderForShadow(m_pointShadowShader);
        }
    }

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupLightUniforms(unsigned int shaderProgram, const Camera& camera) {
    Logger::info("=== setupLightUniforms called ===");
    Logger::info("Total lights: " + std::to_string(m_lights.size()));
    
    // Set lighting uniforms
    int directionalLightCount = 0;
    int pointLightCount = 0;
    int spotLightCount = 0;
    
    // Track shadow-casting lights
    bool directionalShadowSet = false;
    bool spotShadowSet = false;
    bool pointShadowSet = false;
    
    // First pass: count lights and set basic properties
    for (size_t i = 0; i < m_lights.size(); i++) {
        auto& light = m_lights[i];
        std::string baseUniform;
        
        switch (light->type) {
            case LightType::DIRECTIONAL: {
                baseUniform = "directionalLights[" + std::to_string(directionalLightCount) + "].";
                auto* dirLight = static_cast<DirectionalLight*>(light.get());
                
                Logger::info("Directional light " + std::to_string(directionalLightCount) + 
                           " - castShadows: " + std::to_string(dirLight->castShadows));
                
                glUniform3fv(glGetUniformLocation(shaderProgram, (baseUniform + "direction").c_str()), 1, &dirLight->direction[0]);
                glUniform3fv(glGetUniformLocation(shaderProgram, (baseUniform + "color").c_str()), 1, &dirLight->color[0]);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "intensity").c_str()), dirLight->intensity);
                glUniform1i(glGetUniformLocation(shaderProgram, (baseUniform + "castShadows").c_str()), dirLight->castShadows);
                
                // Set single directional shadow map (first directional light that casts shadows)
                if (dirLight->castShadows && m_shadowMaps.find(light.get()) != m_shadowMaps.end() && !directionalShadowSet) {
                    Logger::info("Setting directional shadow map for light " + std::to_string(directionalLightCount));
                    // Bind directional shadow map to a dedicated texture unit to avoid clobbering material textures
                    const int DIR_SHADOW_UNIT = 11;
                    glActiveTexture(GL_TEXTURE0 + DIR_SHADOW_UNIT);
                    glBindTexture(GL_TEXTURE_2D, m_shadowMaps[light.get()]);
                    glUniform1i(glGetUniformLocation(shaderProgram, "shadowMap"), DIR_SHADOW_UNIT);

                    // Supply the cached light-space matrix if available
                    if (m_lightSpaceMatrices.find(light.get()) != m_lightSpaceMatrices.end()) {
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &m_lightSpaceMatrices[light.get()][0][0]);
                    } else {
                        // Fall back to light's own matrix
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &dirLight->getLightSpaceMatrix()[0][0]);
                    }

                    directionalShadowSet = true;
                }
                
                directionalLightCount++;
                break;
            }
            case LightType::SPOT: {
                baseUniform = "spotLights[" + std::to_string(spotLightCount) + "].";
                auto* spotLight = static_cast<SpotLight*>(light.get());
                
                Logger::info("Spot light " + std::to_string(spotLightCount) + 
                           " - castShadows: " + std::to_string(spotLight->castShadows));
                
                glUniform3fv(glGetUniformLocation(shaderProgram, (baseUniform + "position").c_str()), 1, &spotLight->position[0]);
                glUniform3fv(glGetUniformLocation(shaderProgram, (baseUniform + "direction").c_str()), 1, &spotLight->direction[0]);
                glUniform3fv(glGetUniformLocation(shaderProgram, (baseUniform + "color").c_str()), 1, &spotLight->color[0]);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "intensity").c_str()), spotLight->intensity);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "cutOff").c_str()), spotLight->cutOff);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "outerCutOff").c_str()), spotLight->outerCutOff);
                glUniform1i(glGetUniformLocation(shaderProgram, (baseUniform + "castShadows").c_str()), spotLight->castShadows);
                
                // Bind spot shadow map
                const int SPOT_SHADOW_BASE_UNIT = 12;
                if (m_shadowMaps.find(light.get()) != m_shadowMaps.end()) {
                    int unit = SPOT_SHADOW_BASE_UNIT + spotLightCount;
                    glActiveTexture(GL_TEXTURE0 + unit);
                    glBindTexture(GL_TEXTURE_2D, m_shadowMaps[light.get()]);
                    std::string samplerName = "spotShadowMaps[" + std::to_string(spotLightCount) + "]";
                    GLint samplerLoc = glGetUniformLocation(shaderProgram, samplerName.c_str());
                    if (samplerLoc != -1) {
                        glUniform1i(samplerLoc, unit);
                    }
                }

                // Set the per-spot light-space matrix for sampling
                std::string matName = "spotLightSpaceMatrices[" + std::to_string(spotLightCount) + "]";
                GLint matLoc = glGetUniformLocation(shaderProgram, matName.c_str());
                if (matLoc != -1) {
                    glUniformMatrix4fv(matLoc, 1, GL_FALSE, &spotLight->getLightSpaceMatrix()[0][0]);
                }
                
                if (spotLight->castShadows) {
                    Logger::info("Spot light " + std::to_string(spotLightCount) + " - castShadows: 1");
                }
                
                spotLightCount++;
                break;
            }
            case LightType::AREA:
                // Area light shadows are complex - skip for now
                break;
            case LightType::POINT: {
                baseUniform = "pointLights[" + std::to_string(pointLightCount) + "].";
                auto* pointLight = static_cast<PointLight*>(light.get());
                
                Logger::info("Point light " + std::to_string(pointLightCount) + 
                           " - castShadows: " + std::to_string(pointLight->castShadows));
                
                glUniform3fv(glGetUniformLocation(shaderProgram, (baseUniform + "position").c_str()), 1, &pointLight->position[0]);
                glUniform3fv(glGetUniformLocation(shaderProgram, (baseUniform + "color").c_str()), 1, &pointLight->color[0]);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "intensity").c_str()), pointLight->intensity);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "radius").c_str()), pointLight->radius);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "constant").c_str()), pointLight->constant);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "linear").c_str()), pointLight->linear);
                glUniform1f(glGetUniformLocation(shaderProgram, (baseUniform + "quadratic").c_str()), pointLight->quadratic);
                glUniform1i(glGetUniformLocation(shaderProgram, (baseUniform + "castShadows").c_str()), pointLight->castShadows);
                
                // Set point shadow map (first point light that casts shadows)
                if (pointLight->castShadows && m_shadowCubemaps.find(light.get()) != m_shadowCubemaps.end() && !pointShadowSet) {
                    Logger::info("Setting point shadow cubemap for light " + std::to_string(pointLightCount));
                    // Bind point shadow cubemap to a high texture unit to avoid clobbering material textures
                    glActiveTexture(GL_TEXTURE10);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, m_shadowCubemaps[light.get()]);
                    glUniform1i(glGetUniformLocation(shaderProgram, "pointShadowMap"), 10);
                    glUniform1f(glGetUniformLocation(shaderProgram, "pointFarPlane"), pointLight->radius);
                    pointShadowSet = true;
                }

                pointLightCount++;
                break;
            }
        }
    }
    
    // Set light counts
    glUniform1i(glGetUniformLocation(shaderProgram, "directionalLightCount"), directionalLightCount);
    glUniform1i(glGetUniformLocation(shaderProgram, "pointLightCount"), pointLightCount);
    glUniform1i(glGetUniformLocation(shaderProgram, "spotLightCount"), spotLightCount);
    
    Logger::info("Light counts - Directional: " + std::to_string(directionalLightCount) +
                ", Point: " + std::to_string(pointLightCount) +
                ", Spot: " + std::to_string(spotLightCount));
    
    // Camera position
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 
                camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
    
    Logger::info("=== setupLightUniforms finished ===");
}

void Renderer::renderLightVisualization(const Camera& camera, Light* light) {
    unsigned int lightShader = m_shaderManager->getShader("light");
    glUseProgram(lightShader);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f/9.0f, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(lightShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(lightShader, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    glm::vec3 lightPos = light->getPosition();
    glm::vec3 lightColor = light->color;
    float lightScale = 0.3f;
    
    if (light->type == LightType::DIRECTIONAL) {
        lightScale = 0.5f;
    } else if (light->type == LightType::SPOT) {
        lightScale = 0.4f;
    }
    
    glUniform3f(glGetUniformLocation(lightShader, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(lightShader, "lightColor"), lightColor.x, lightColor.y, lightColor.z);
    glUniform1f(glGetUniformLocation(lightShader, "lightScale"), lightScale);

    glBindVertexArray(m_lightVAO);
    // Draw the cube as triangles (36 vertices)
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::renderScene(const Camera& camera) {
    // First pass: render ALL shadow maps
    renderShadowMaps();

    // Second pass: main rendering
    glViewport(0, 0, 1280, 720);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    unsigned int pbrShader = m_shaderManager->getShader("pbr");
    Logger::info(std::string("PBR shader program id = ") + std::to_string(pbrShader));

    // If debug visualization is enabled, render the first texture to fullscreen and skip the PBR pass
    if (m_showDebugAlbedo && m_debugQuad) {
        unsigned int debugShader = m_shaderManager->getShader("debug_quad");
        unsigned int tex = m_textureManager->getFirstTexture();
        Logger::info("Debug albedo pass: texture ID = " + std::to_string(tex));
        if (tex != 0 && debugShader != 0) {
            m_debugQuad->render(debugShader, tex);
            return;
        }
    }

    glUseProgram(pbrShader);
    GLint currentProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProg);
    Logger::info(std::string("glUseProgram called, current program = ") + std::to_string(currentProg));

    // Set common uniforms
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f/9.0f, 0.1f, 100.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(pbrShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pbrShader, "projection"), 1, GL_FALSE, &projection[0][0]);

    // Setup all light uniforms (including shadows)
    setupLightUniforms(pbrShader, camera);

    // Render ALL scene objects
    if (m_scene) {
        m_scene->render(pbrShader);
    }

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::error(std::string("GL error after scene render: ") + std::to_string(err));
    }

    // Render the visible light sources
    renderLightSources(camera);
}

void Renderer::beginFrame() {
    clearScreen();
}

void Renderer::endFrame() {
}

void Renderer::clearScreen() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::shutdown() {
    // Clean up all shadow resources
    for (auto& light : m_lights) {
        cleanupLightShadow(light.get());
    }
    m_shadowFBOs.clear();
    m_shadowMaps.clear();
    m_shadowCubemaps.clear();

    if (m_shaderManager) {
        m_shaderManager->cleanup();
    }
    
    if (m_textureManager) {
        m_textureManager->cleanup();
    }
    
    if (m_lightVAO) glDeleteVertexArrays(1, &m_lightVAO);
    if (m_lightVBO) glDeleteBuffers(1, &m_lightVBO);

    m_lights.clear();
    m_initialized = false;
}

void Renderer::renderLightSources(const Camera& camera) {
    for (auto& light : m_lights) {
        renderLightVisualization(camera, light.get());
    }
}

void Renderer::setScene(Scene* scene) {
    m_scene = scene;
    if (m_scene) {
        Logger::info(std::string("Renderer::setScene called - scene set at ") + std::to_string((uintptr_t)m_scene));
    } else {
        Logger::warn("Renderer::setScene called with null scene");
    }
}
