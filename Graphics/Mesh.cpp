#include "Mesh.hpp"
#include <iostream>
#include "../Utils/Logger.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices, 
           const std::vector<unsigned int>& indices, 
           const std::vector<Texture>& textures,
           const PBRMaterial& material)
    : m_vertices(vertices)
    , m_indices(indices)
    , m_textures(textures)
    , m_material(material)
    , m_VAO(0), m_VBO(0), m_EBO(0) {  // Initialize to 0
    
    // DON'T call setupMesh here - wait until first draw
    // setupMesh();
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
}

size_t Mesh::getIndexCount() const {
    return m_indices.size();
}

void checkGLError(const std::string& context) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        Logger::error("OpenGL error in " + context + ": " + std::to_string(error));
    }
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    checkGLError("Mesh buffer generation");
    Logger::info("Mesh::setupMesh - gen buffers VAO=" + std::to_string(m_VAO) + ", VBO=" + std::to_string(m_VBO) + ", EBO=" + std::to_string(m_EBO));

    glBindVertexArray(m_VAO);
    checkGLError("Mesh bind VAO");
    Logger::info("Mesh::setupMesh - bound VAO=" + std::to_string(m_VAO));

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), 
                 &m_vertices[0], GL_STATIC_DRAW);
    Logger::info("Mesh::setupMesh - VBO data uploaded (vertices=" + std::to_string(m_vertices.size()) + ")");
    
    // Element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), 
                 &m_indices[0], GL_STATIC_DRAW);
    Logger::info("Mesh::setupMesh - EBO data uploaded (indices=" + std::to_string(m_indices.size()) + ")");
    
    // Vertex attributes
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    Logger::info("Mesh::setupMesh - attrib 0 position set");
    
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    Logger::info("Mesh::setupMesh - attrib 1 normal set");
    
    // Texture coordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    Logger::info("Mesh::setupMesh - attrib 2 texCoords (TEXCOORD_0) set");
    // Second UV set (TEXCOORD_1)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords1));
    Logger::info("Mesh::setupMesh - attrib 3 texCoords1 (TEXCOORD_1) set");
    
    // Tangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    Logger::info("Mesh::setupMesh - attrib 4 tangent set");
    
    // Bitangent
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    Logger::info("Mesh::setupMesh - attrib 5 bitangent set");
    
    glBindVertexArray(0);
    checkGLError("Mesh buffer generation");
    Logger::info("Mesh::setupMesh completed for VAO=" + std::to_string(m_VAO));
}

void Mesh::draw(unsigned int shaderProgram, const glm::mat4& modelMatrix) {
    // Logger::info("Mesh::draw called - " + std::to_string(m_vertices.size()) + " vertices, " + 
    //              std::to_string(m_indices.size()) + " indices");
    // Ensure the shader program is active before setting uniforms
    glUseProgram(shaderProgram);
    
    // LAZY INITIALIZATION: Create buffers on first draw
    if (m_VAO == 0) {
        Logger::info("Creating OpenGL buffers for mesh...");
        setupMesh();
    }
    
    // Check if VAO is valid
    if (m_VAO == 0) {
        Logger::error("Invalid VAO in Mesh::draw!");
        return;
    }
    
    // Logger::info("Binding textures...");
    // Default: no maps
    int useAlbedo = 0, useNormal = 0, useMR = 0, useAO = 0, useEmission = 0;
    int albedoUnit = -1, normalUnit = -1, mrUnit = -1, aoUnit = -1, emissionUnit = -1;

    unsigned int texUnit = 0;
    
    // Check if mesh has second UV set - if not, force all textures to use UV set 0
    bool hasTexCoords1 = this->hasTexCoords1();
    
    for (unsigned int i = 0; i < m_textures.size(); i++) {
        const auto& t = m_textures[i];
        if (t.type == "albedo" && !useAlbedo) {
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, t.id);
            glUniform1i(glGetUniformLocation(shaderProgram, "albedoMap"), texUnit);
            useAlbedo = 1; albedoUnit = texUnit; texUnit++;
            // set UV transform and UV set
            glUniform2f(glGetUniformLocation(shaderProgram, "albedoUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "albedoUVOffset"), t.uvOffset.x, t.uvOffset.y);
            int actualUVSet = (t.uvSet == 1 && !hasTexCoords1) ? 0 : t.uvSet;  // Force to 0 if no texCoords1
            glUniform1i(glGetUniformLocation(shaderProgram, "albedoUVSet"), actualUVSet);
            // Logger::info("  Bound albedo map to unit " + std::to_string(albedoUnit));
        }
        else if (t.type == "normal" && !useNormal) {
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, t.id);
            glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), texUnit);
            useNormal = 1; normalUnit = texUnit; texUnit++;
            glUniform2f(glGetUniformLocation(shaderProgram, "normalUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "normalUVOffset"), t.uvOffset.x, t.uvOffset.y);
            int actualUVSet = (t.uvSet == 1 && !hasTexCoords1) ? 0 : t.uvSet;  // Force to 0 if no texCoords1
            glUniform1i(glGetUniformLocation(shaderProgram, "normalUVSet"), actualUVSet);
            // Logger::info("  Bound normal map to unit " + std::to_string(normalUnit));
        }
        else if (t.type == "metallicRoughness" && !useMR) {
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, t.id);
            glUniform1i(glGetUniformLocation(shaderProgram, "metallicRoughnessMap"), texUnit);
            useMR = 1; mrUnit = texUnit; texUnit++;
            glUniform2f(glGetUniformLocation(shaderProgram, "metallicRoughnessUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "metallicRoughnessUVOffset"), t.uvOffset.x, t.uvOffset.y);
            int actualUVSet = (t.uvSet == 1 && !hasTexCoords1) ? 0 : t.uvSet;  // Force to 0 if no texCoords1
            glUniform1i(glGetUniformLocation(shaderProgram, "metallicRoughnessUVSet"), actualUVSet);
            // Logger::info("  Bound metallicRoughness map to unit " + std::to_string(mrUnit));
        }
        else if (t.type == "ao" && !useAO) {
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, t.id);
            glUniform1i(glGetUniformLocation(shaderProgram, "aoMap"), texUnit);
            useAO = 1; aoUnit = texUnit; texUnit++;
            glUniform2f(glGetUniformLocation(shaderProgram, "aoUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "aoUVOffset"), t.uvOffset.x, t.uvOffset.y);
            int actualUVSet = (t.uvSet == 1 && !hasTexCoords1) ? 0 : t.uvSet;  // Force to 0 if no texCoords1
            glUniform1i(glGetUniformLocation(shaderProgram, "aoUVSet"), actualUVSet);
            // Logger::info("  Bound ao map to unit " + std::to_string(aoUnit));
        }
        else if (t.type == "emission" && !useEmission) {
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, t.id);
            glUniform1i(glGetUniformLocation(shaderProgram, "emissionMap"), texUnit);
            useEmission = 1; emissionUnit = texUnit; texUnit++;
            glUniform2f(glGetUniformLocation(shaderProgram, "emissionUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "emissionUVOffset"), t.uvOffset.x, t.uvOffset.y);
            int actualUVSet = (t.uvSet == 1 && !hasTexCoords1) ? 0 : t.uvSet;  // Force to 0 if no texCoords1
            glUniform1i(glGetUniformLocation(shaderProgram, "emissionUVSet"), actualUVSet);
            // Logger::info("  Bound emission map to unit " + std::to_string(emissionUnit));
        }
        else {
            // ignore other textures for now
        }
    }

    // Set flags in shader
    glUniform1i(glGetUniformLocation(shaderProgram, "useAlbedoMap"), useAlbedo);
    glUniform1i(glGetUniformLocation(shaderProgram, "useNormalMap"), useNormal);
    glUniform1i(glGetUniformLocation(shaderProgram, "useMetallicRoughnessMap"), useMR);
    glUniform1i(glGetUniformLocation(shaderProgram, "useAOMap"), useAO);
    glUniform1i(glGetUniformLocation(shaderProgram, "useEmissionMap"), useEmission);
    
    // Logger::info("Setting uniforms...");
    // Set material properties
    glUniform3f(glGetUniformLocation(shaderProgram, "albedo"), 
                m_material.albedo.x, m_material.albedo.y, m_material.albedo.z);
    glUniform1f(glGetUniformLocation(shaderProgram, "metallic"), m_material.metallic);
    glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), m_material.roughness);
    glUniform1f(glGetUniformLocation(shaderProgram, "ao"), m_material.ao);
    glUniform3f(glGetUniformLocation(shaderProgram, "emission"), 
                m_material.emission.x, m_material.emission.y, m_material.emission.z);
    glUniform1f(glGetUniformLocation(shaderProgram, "emissionStrength"), m_material.emissionStrength);
    
    // Set UV transforms per texture type (from glTF KHR_texture_transform)
    for (unsigned int i = 0; i < m_textures.size(); i++) {
        const auto& t = m_textures[i];
        if (t.type == "albedo") {
            glUniform2f(glGetUniformLocation(shaderProgram, "albedoUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "albedoUVOffset"), t.uvOffset.x, t.uvOffset.y);
        }
        else if (t.type == "normal") {
            glUniform2f(glGetUniformLocation(shaderProgram, "normalUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "normalUVOffset"), t.uvOffset.x, t.uvOffset.y);
        }
        else if (t.type == "metallicRoughness") {
            glUniform2f(glGetUniformLocation(shaderProgram, "metallicRoughnessUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "metallicRoughnessUVOffset"), t.uvOffset.x, t.uvOffset.y);
        }
        else if (t.type == "ao") {
            glUniform2f(glGetUniformLocation(shaderProgram, "aoUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "aoUVOffset"), t.uvOffset.x, t.uvOffset.y);
        }
        else if (t.type == "emission") {
            glUniform2f(glGetUniformLocation(shaderProgram, "emissionUVScale"), t.uvScale.x, t.uvScale.y);
            glUniform2f(glGetUniformLocation(shaderProgram, "emissionUVOffset"), t.uvOffset.x, t.uvOffset.y);
        }
    }
    
    // Set model matrix
    
    // Logger::info("  Row 0: [" + std::to_string(modelMatrix[0][0]) + ", " + std::to_string(modelMatrix[0][1]) + ", " + std::to_string(modelMatrix[0][2]) + ", " + std::to_string(modelMatrix[0][3]) + "]");
    // Logger::info("  Row 1: [" + std::to_string(modelMatrix[1][0]) + ", " + std::to_string(modelMatrix[1][1]) + ", " + std::to_string(modelMatrix[1][2]) + ", " + std::to_string(modelMatrix[1][3]) + "]");
    // Logger::info("  Row 2: [" + std::to_string(modelMatrix[2][0]) + ", " + std::to_string(modelMatrix[2][1]) + ", " + std::to_string(modelMatrix[2][2]) + ", " + std::to_string(modelMatrix[2][3]) + "]");
    // Logger::info("  Row 3: [" + std::to_string(modelMatrix[3][0]) + ", " + std::to_string(modelMatrix[3][1]) + ", " + std::to_string(modelMatrix[3][2]) + ", " + std::to_string(modelMatrix[3][3]) + "]");
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
    
    // Logger::info("Calling glDrawElements...");
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Logger::info("Mesh::draw completed successfully");
}

// Check if mesh has second UV set (any vertex with non-zero texCoords1)
bool Mesh::hasTexCoords1() const {
    if (m_vertices.empty()) {
        return false;
    }
    
    // Check if ANY vertex has non-zero texCoords1
    for (const auto& v : m_vertices) {
        if (v.texCoords1 != glm::vec2(0.0f, 0.0f)) {
            return true;
        }
    }
    return false;
}

void Mesh::drawForShadow(unsigned int shadowShader, const glm::mat4& modelMatrix) {
    if (m_VAO == 0) {
        setupMesh();
    }
    
    glUseProgram(shadowShader);
    glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
    
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}