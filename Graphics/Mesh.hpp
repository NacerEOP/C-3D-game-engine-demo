#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../include/glad/glad.h"
#include "Materials/PBRMaterial.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec2 texCoords1 = glm::vec2(0.0f, 0.0f);
    // Make these optional or provide defaults
    glm::vec3 tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
    // glTF KHR_texture_transform support
    glm::vec2 uvScale = glm::vec2(1.0f, 1.0f);
    glm::vec2 uvOffset = glm::vec2(0.0f, 0.0f);
    // which TEXCOORD set to use (0 or 1)
    int uvSet = 0;
};

class Mesh {
protected:  // CHANGE FROM private TO protected
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<Texture> m_textures;
    PBRMaterial m_material;
    
    unsigned int m_VAO, m_VBO, m_EBO;

public:
    Mesh(const std::vector<Vertex>& vertices, 
         const std::vector<unsigned int>& indices, 
         const std::vector<Texture>& textures,
         const PBRMaterial& material);
    ~Mesh();
    
    void draw(unsigned int shaderProgram, const glm::mat4& modelMatrix);
    const PBRMaterial& getMaterial() const { return m_material; }
    // Return number of indices for diagnostics
    size_t getIndexCount() const;
    
    // Check if mesh has second UV set
    bool hasTexCoords1() const;
    
    // ADD THIS METHOD:
    void drawForShadow(unsigned int shadowShader, const glm::mat4& modelMatrix);  // ADD THIS

    
protected:  // CHANGE FROM private TO protected
    void setupMesh();
};