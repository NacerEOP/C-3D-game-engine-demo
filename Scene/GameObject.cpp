#include "GameObject.hpp"
#include "../Utils/Logger.hpp"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>

GameObject::GameObject(const std::string& name) 
    : m_name(name)
    , m_position(0.0f)
    , m_rotation(0.0f)
    , m_scale(1.0f)
    , m_visible(true) {
}

GameObject::~GameObject() {
}

void GameObject::loadModel(const std::string& path, TextureManager& textureManager) {
    Logger::info("GameObject::loadModel called for: " + m_name + " with path: " + path);
    m_model = std::make_unique<Model>(path, textureManager);
    Logger::info("Loaded model for GameObject: " + m_name);
}

void GameObject::setManualMesh(const std::vector<Vertex>& vertices, 
                               const std::vector<unsigned int>& indices,
                               const PBRMaterial& material) {
    Logger::info("GameObject::setManualMesh called for: " + m_name + " with " + 
                 std::to_string(vertices.size()) + " vertices, " + 
                 std::to_string(indices.size()) + " indices");
    m_manualMesh = std::make_unique<ManualMesh>(vertices, indices, material);
    Logger::info("Created manual mesh for GameObject: " + m_name);
}

glm::mat4 GameObject::getModelMatrix() const {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, m_position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, m_scale);
    return modelMatrix;
}

void GameObject::draw(unsigned int shaderProgram) {
    Logger::info(std::string("GameObject::draw called for: ") + m_name + " visible=" + std::to_string(m_visible));
    if (m_visible) {
        if (m_model) {
            m_model->setPosition(m_position);
            m_model->setRotation(m_rotation);
            m_model->setScale(m_scale);
            m_model->draw(shaderProgram);
        } 
        else if (m_manualMesh) {
            glm::mat4 modelMatrix = getModelMatrix();
            
            // Ensure shader program is active before setting uniforms
            glUseProgram(shaderProgram);

            // MANUALLY SET MATERIAL UNIFORMS FOR MANUAL MESH
            glUniform3f(glGetUniformLocation(shaderProgram, "albedo"), 
                        m_manualMesh->getMaterial().albedo.x, 
                        m_manualMesh->getMaterial().albedo.y, 
                        m_manualMesh->getMaterial().albedo.z);
            glUniform1f(glGetUniformLocation(shaderProgram, "metallic"), m_manualMesh->getMaterial().metallic);
            glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), m_manualMesh->getMaterial().roughness);
            glUniform1f(glGetUniformLocation(shaderProgram, "ao"), m_manualMesh->getMaterial().ao);
            
            m_manualMesh->draw(shaderProgram, modelMatrix);
        } 
        else {
            Logger::warn("GameObject " + m_name + " has no model or manual mesh!");
        }
    } else {
        Logger::warn("GameObject " + m_name + " not visible");
    }
}

void GameObject::drawForShadow(unsigned int shadowShader) {
    if (!m_visible) return;
    
    // Logger::info("GameObject " + m_name + " drawing for shadow");
    
    glm::mat4 modelMatrix = getModelMatrix();
    
    if (m_model) {
        // Logger::info("  - Using model for shadow: " + m_name);
        m_model->drawForShadow(shadowShader, modelMatrix);
    } 
    else if (m_manualMesh) {
        // Logger::info("  - Using manual mesh for shadow: " + m_name);
        m_manualMesh->drawForShadow(shadowShader, modelMatrix);
    } else {
        Logger::warn("  - No model or manual mesh for shadow: " + m_name);
    }
}

void GameObject::setPosition(const glm::vec3& position) {
    m_position = position;
}

void GameObject::setRotation(const glm::vec3& rotation) {
    m_rotation = rotation;
}

void GameObject::setScale(const glm::vec3& scale) {
    m_scale = scale;
}

void GameObject::move(const glm::vec3& offset) {
    m_position += offset;
}

void GameObject::rotate(const glm::vec3& rotation) {
    m_rotation += rotation;
}