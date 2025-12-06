#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "../Graphics/Camera.hpp"
#include "../Graphics/TextureManager.hpp"
#include "GameObject.hpp"

class Scene {
private:
    Camera m_camera;
    TextureManager m_textureManager;
    std::unordered_map<std::string, std::unique_ptr<GameObject>> m_gameObjects;
    
public:
    Scene();
    
    void initialize();
    void loadModels();
    void update(float deltaTime);
    void render(unsigned int shaderProgram);
    
    GameObject* createGameObject(const std::string& name);
    GameObject* getGameObject(const std::string& name);
    void removeGameObject(const std::string& name);
    
    Camera& getCamera() { return m_camera; }
    const Camera& getCamera() const { return m_camera; }
    TextureManager& getTextureManager() { return m_textureManager; }

    void renderForShadow(unsigned int shadowShader);

    // Compute an approximate scene bounding sphere (center, radius) from object positions
    void computeSceneBounds(glm::vec3& outCenter, float& outRadius) const;

private:
    void createDefaultScene();
};