#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>  // ADD THIS
#include <glm/glm.hpp>
#include "Mesh.hpp"
#include "TextureManager.hpp"
#include "Materials/PBRMaterial.hpp"

// Structure to hold texture transform data (from glTF KHR_texture_transform)
struct TextureTransform {
    glm::vec2 scale = glm::vec2(1.0f, 1.0f);
    glm::vec2 offset = glm::vec2(0.0f, 0.0f);
    float rotation = 0.0f;
    // which TEXCOORD set specified in glTF (0 if absent)
    int texCoord = 0;
    
    TextureTransform() = default;
    TextureTransform(glm::vec2 s, glm::vec2 o) : scale(s), offset(o) {}
};

class Model {
private:
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::string m_directory;
    std::string m_modelPath;
    TextureManager& m_textureManager;
    bool m_gammaCorrection;

    std::unordered_map<std::string, unsigned int> m_texturesLoaded;
    
    // Cache for glTF texture transforms: key = "material_index,texture_index"
    std::unordered_map<std::string, TextureTransform> m_textureTransforms;

public:
    Model(const std::string& path, TextureManager& textureManager, bool gamma = false);
    ~Model();

    void draw(unsigned int shaderProgram);
    void setTransform(const glm::mat4& transform);

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& rotation);
    void setScale(const glm::vec3& scale);
    void drawForShadow(unsigned int shadowShader, const glm::mat4& modelMatrix);  // ADD THIS

private:
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
    
    PBRMaterial loadMaterial(aiMaterial* mat);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const aiScene* scene, int materialIndex = -1);
    
    // glTF JSON parsing helpers
    std::string extractGltfJson(const std::string& path);
    void parseGltfTextures(const std::string& jsonStr, int materialIndex);
#ifdef USE_NLOHMANN
    void parseGltfTexturesJson(const std::string& jsonStr, int materialIndex);
#endif
    TextureTransform parseTextureTransform(const std::string& textureSection);
    
    glm::mat4 m_transform;
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::vec3 m_scale;
    float m_vertexScale;

    void updateTransform();
};