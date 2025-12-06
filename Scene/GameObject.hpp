#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "../Graphics/Model.hpp"
#include "../Graphics/ManualMesh.hpp"

class GameObject {
private:
    std::string m_name;
    std::unique_ptr<Model> m_model;
    std::unique_ptr<ManualMesh> m_manualMesh;
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::vec3 m_scale;
    bool m_visible;

public:
    GameObject(const std::string& name);
    ~GameObject();
    
    void loadModel(const std::string& path, TextureManager& textureManager);
    void setManualMesh(const std::vector<Vertex>& vertices, 
                       const std::vector<unsigned int>& indices,
                       const PBRMaterial& material);
    
    void draw(unsigned int shaderProgram);
    void drawForShadow(unsigned int shadowShader);  // SINGLE DECLARATION
    
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& rotation);
    void setScale(const glm::vec3& scale);
    void move(const glm::vec3& offset);
    void rotate(const glm::vec3& rotation);
    
    const std::string& getName() const { return m_name; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getRotation() const { return m_rotation; }
    const glm::vec3& getScale() const { return m_scale; }
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    bool hasModel() const { return m_model != nullptr; }
    bool hasManualMesh() const { return m_manualMesh != nullptr; }
    
    glm::mat4 getModelMatrix() const;
};