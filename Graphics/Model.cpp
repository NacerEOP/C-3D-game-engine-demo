#include "Model.hpp"
#include "../Utils/Logger.hpp"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <fstream>
#include <memory>  // ADD THIS
#include <sstream>

// Optionally use nlohmann::json if available. Define USE_NLOHMANN in your build
// (e.g. -DUSE_NLOHMANN) and ensure the header is on the include path.
#ifdef USE_NLOHMANN
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

// (aiMatrix4x4ToGlm removed — using PreTransformVertices flattening)

Model::Model(const std::string& path, TextureManager& textureManager, bool gamma) 
    : m_textureManager(textureManager)
    , m_gammaCorrection(gamma)
    , m_transform(1.0f)
    , m_position(0.0f)
    , m_rotation(0.0f)
    , m_scale(1.0f) {
    
    Logger::info("\n\n========== MODEL CONSTRUCTOR ==========");
    Logger::info("Loading model: " + path);
    
    std::ifstream file(path);
    if (!file.good()) {
        Logger::error("Model file does not exist: " + path);
        return;
    }
    file.close();
    
    try {
        loadModel(path);
    } catch (const std::exception& ex) {
        Logger::error(std::string("Exception during Model::loadModel: ") + ex.what());
    } catch (...) {
        Logger::error("Unknown exception during Model::loadModel");
    }
    Logger::info("========== MODEL CONSTRUCTOR COMPLETE ==========\n\n");
}

Model::~Model() {
    m_meshes.clear();
    m_texturesLoaded.clear();
    Logger::info("Model destroyed: " + m_directory);
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_GenNormals |
        aiProcess_PreTransformVertices);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::error("ASSIMP ERROR: " + std::string(importer.GetErrorString()));
        return;
    }
    
    Logger::info("Scene loaded with " + std::to_string(scene->mNumMaterials) + " materials");
    Logger::info("Scene has " + std::to_string(scene->mNumMeshes) + " meshes");
    
    // Compute directory robustly on Windows and Unix (handle both '/' and '\\')
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        m_directory = ".";
    } else {
        m_directory = path.substr(0, pos);
    }
    // Keep full model path so embedded texture keys are unique per model
    m_modelPath = path;
    // Decide per-model normalization: only apply the previous 1/200 normalization
    // for the mclaren model to avoid shrinking other models (like the chair).
    if (path.find("mclaren") != std::string::npos) {
        m_vertexScale = 1.0f / 200.0f;
        Logger::info("Applying vertex normalization scale " + std::to_string(m_vertexScale) + " for " + path);
    } else {
        m_vertexScale = 1.0f;
    }
    
    // Extract and parse glTF JSON if this is a .glb file
    std::string gltfJson = extractGltfJson(path);
    if (!gltfJson.empty()) {
        // For debugging: dump the glTF JSON only for the classroom model so we can inspect texture transforms
        if (path.find("classroom") != std::string::npos || path.find("ClassRoom") != std::string::npos) {
            std::ofstream jsonDebug("gltf_debug_classroom.json");
            if (jsonDebug.is_open()) {
                jsonDebug << gltfJson;
                jsonDebug.close();
                Logger::info("Wrote glTF JSON debug dump: gltf_debug_classroom.json");
            }
        }

        // Parse texture transforms for all materials
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
#ifdef USE_NLOHMANN
            parseGltfTexturesJson(gltfJson, i);
#else
            parseGltfTextures(gltfJson, i);
#endif
        }
    }

    processNode(scene->mRootNode, scene);
    Logger::info("Loaded model: " + path + " with " + std::to_string(m_meshes.size()) + " meshes");
}
void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

std::unique_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Logger::info("Processing mesh: " + std::string(mesh->mName.C_Str()) + 
                 " with " + std::to_string(mesh->mNumVertices) + " vertices");
    
    // Validate mesh data before processing
    if (mesh->mNumVertices == 0 || !mesh->mVertices) {
        Logger::error("ERROR: Mesh " + std::string(mesh->mName.C_Str()) + " has no vertices!");
        return nullptr;
    }
    
    if (mesh->mNumFaces == 0 || !mesh->mFaces) {
        Logger::error("ERROR: Mesh " + std::string(mesh->mName.C_Str()) + " has no faces!");
        return nullptr;
    }
    
    Logger::info("  HasNormals: " + std::string(mesh->HasNormals() ? "yes" : "no"));
    Logger::info("  TextureCoords[0]: " + std::string((mesh->mTextureCoords && mesh->mTextureCoords[0]) ? "yes" : "no"));
    Logger::info("  NumFaces: " + std::to_string(mesh->mNumFaces));

    // Track vertex bounds
    glm::vec3 minPos(FLT_MAX), maxPos(FLT_MIN);
    // Track UV bounds to detect >1.0 tiling or unexpected normalization
    glm::vec2 minUV(FLT_MAX), maxUV(FLT_MIN);
    
    // Process vertices (apply model-specific vertex scale)
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
        // Position (apply normalization)
        vertex.position = glm::vec3(
            mesh->mVertices[i].x * m_vertexScale,
            mesh->mVertices[i].y * m_vertexScale, 
            mesh->mVertices[i].z * m_vertexScale
        );
        
        minPos = glm::min(minPos, vertex.position);
        maxPos = glm::max(maxPos, vertex.position);
        
        // Log first few vertices to see actual coordinate ranges
        if (i < 3) {
            Logger::info("    Vertex " + std::to_string(i) + " position: (" + 
                        std::to_string(vertex.position.x) + ", " + 
                        std::to_string(vertex.position.y) + ", " + 
                        std::to_string(vertex.position.z) + ")");
        }
        
        // Normal
        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        
        // Texture coordinates
        if (mesh->mTextureCoords && mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
            // Also read second UV set if available
            if (mesh->mTextureCoords[1]) {
                vertex.texCoords1 = glm::vec2(
                    mesh->mTextureCoords[1][i].x,
                    mesh->mTextureCoords[1][i].y
                );
            } else {
                vertex.texCoords1 = glm::vec2(0.0f, 0.0f);
            }
            minUV = glm::min(minUV, vertex.texCoords);
            maxUV = glm::max(maxUV, vertex.texCoords);
        } else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
            minUV = glm::min(minUV, vertex.texCoords);
            maxUV = glm::max(maxUV, vertex.texCoords);
        }
        
        vertices.push_back(vertex);
    }
    
    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    // Process material
    PBRMaterial material;
    if (mesh->mMaterialIndex >= 0) {
        Logger::info("  Material index: " + std::to_string(mesh->mMaterialIndex));
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        material = loadMaterial(mat);

        // Load textures (PBR): base color, normal, metallic-roughness, ao
        // Pass material index so we can look up glTF texture transforms
        auto albedoMaps = loadMaterialTextures(mat, aiTextureType_BASE_COLOR, "albedo", scene, mesh->mMaterialIndex);
        textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());

        auto normalMaps = loadMaterialTextures(mat, aiTextureType_NORMALS, "normal", scene, mesh->mMaterialIndex);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // metallic-roughness can be stored in METALNESS or DIFFUSE_ROUGHNESS depending on exporter
        auto mrMaps = loadMaterialTextures(mat, aiTextureType_METALNESS, "metallicRoughness", scene, mesh->mMaterialIndex);
        if (mrMaps.empty()) {
            auto alt = loadMaterialTextures(mat, aiTextureType_DIFFUSE_ROUGHNESS, "metallicRoughness", scene, mesh->mMaterialIndex);
            mrMaps.insert(mrMaps.end(), alt.begin(), alt.end());
        }
        textures.insert(textures.end(), mrMaps.begin(), mrMaps.end());

        auto aoMaps = loadMaterialTextures(mat, aiTextureType_AMBIENT_OCCLUSION, "ao", scene, mesh->mMaterialIndex);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

        auto emissionMaps = loadMaterialTextures(mat, aiTextureType_EMISSION_COLOR, "emission", scene, mesh->mMaterialIndex);
        textures.insert(textures.end(), emissionMaps.begin(), emissionMaps.end());
    } else {
        Logger::info("  No material assigned, using defaults");
    }

    Logger::info("Mesh processed successfully - bounds: min(" + std::to_string(minPos.x) + "," + std::to_string(minPos.y) + "," + std::to_string(minPos.z) + ") max(" + std::to_string(maxPos.x) + "," + std::to_string(maxPos.y) + "," + std::to_string(maxPos.z) + ")");
    Logger::info("Mesh UV bounds: min(" + std::to_string(minUV.x) + "," + std::to_string(minUV.y) + ") max(" + std::to_string(maxUV.x) + "," + std::to_string(maxUV.y) + ")");
    Logger::info("Mesh final counts: vertices=" + std::to_string(vertices.size()) + ", indices=" + std::to_string(indices.size()) + ", textures=" + std::to_string(textures.size()));
    return std::make_unique<Mesh>(vertices, indices, textures, material);
}

PBRMaterial Model::loadMaterial(aiMaterial* mat) {
    PBRMaterial material;
    
    // Get material name for logging
    aiString matName;
    mat->Get(AI_MATKEY_NAME, matName);
    std::string matNameStr = matName.C_Str();
    Logger::info("=== Loading material: " + matNameStr + " ===");
    
    // For glTF, the base color comes from pbrMetallicRoughness.baseColorFactor
    // ASSIMP should map this to AI_MATKEY_BASE_COLOR
    aiColor3D color(1.0f, 1.0f, 1.0f);  // Default to white
    float value = 0.5f;
    
    // ===== ALBEDO / BASE COLOR =====
    if (mat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
        material.albedo = glm::vec3(color.r, color.g, color.b);
        Logger::info("  Albedo (BASE_COLOR): " + std::to_string(color.r) + ", " + std::to_string(color.g) + ", " + std::to_string(color.b));
        
        // If base color is default white (meaning no explicit color set), check for baseColorTexture
        if (color.r > 0.99f && color.g > 0.99f && color.b > 0.99f) {
            Logger::info("  Base color is white - checking for baseColorTexture");
            if (mat->GetTextureCount(aiTextureType_BASE_COLOR) > 0) {
                // There's a base color texture, but we can't easily sample it here
                // For black leather, assume dark color
                material.albedo = glm::vec3(0.2f, 0.2f, 0.2f);
                Logger::info("  Found BASE_COLOR texture - using dark color for black leather");
            }
        }
    }
    else {
        // For a black leather chair, use dark color instead of white
        material.albedo = glm::vec3(0.1f, 0.1f, 0.1f);
        Logger::info("  Albedo (DEFAULT BLACK): 0.1, 0.1, 0.1");
    }
    
    // ===== METALLIC =====
    // For glTF, metallicFactor is in pbrMetallicRoughness.metallicFactor
    value = 0.0f;
    if (mat->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS) {
        material.metallic = glm::clamp(value, 0.0f, 1.0f);
        Logger::info("  Metallic: " + std::to_string(material.metallic));
    }
    else {
        material.metallic = 0.0f;
        Logger::info("  Metallic (DEFAULT): 0.0");
    }
    
    // ===== ROUGHNESS =====
    // For glTF, roughnessFactor is in pbrMetallicRoughness.roughnessFactor
    // ASSIMP stores this in a custom key: $mat.roughnessFactor
    value = 0.5f;
    if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS) {
        material.roughness = glm::clamp(value, 0.0f, 1.0f);
        Logger::info("  Roughness: " + std::to_string(material.roughness));
    }
    else {
        // Try alternate key
        if (mat->Get("$mat.roughnessFactor", 0, 0, value) == AI_SUCCESS) {
            material.roughness = glm::clamp(value, 0.0f, 1.0f);
            Logger::info("  Roughness (custom key): " + std::to_string(material.roughness));
        }
        else {
            // Leather is typically medium-high roughness
            material.roughness = 0.7f;
            Logger::info("  Roughness (DEFAULT for leather): 0.7");
        }
    }
    
    // ===== AMBIENT OCCLUSION =====
    material.ao = 1.0f;
    Logger::info("  AO (always 1.0 for now)");

    // ===== EMISSION =====
    // Try to get emission factor and color from material
    color = aiColor3D(0.0f, 0.0f, 0.0f);
    if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
        material.emission = glm::vec3(color.r, color.g, color.b);
        Logger::info("  Emission Color: " + std::to_string(color.r) + ", " + std::to_string(color.g) + ", " + std::to_string(color.b));
    } else {
        material.emission = glm::vec3(0.0f, 0.0f, 0.0f);
        Logger::info("  Emission Color (DEFAULT): 0.0, 0.0, 0.0");
    }
    
    // Check if there's an emission texture - if so, set strength to 1.0 for unlit material
    if (mat->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
        Logger::info("  Has emissive texture - setting strength to 1.0 (unlit material)");
        material.emissionStrength = 1.0f;
    }
    // Check if material has explicit emissive color (non-black)
    else if (material.emission.r > 0.1f || material.emission.g > 0.1f || material.emission.b > 0.1f) {
        Logger::info("  Material has explicit emissive color - setting strength to 1.0");
        material.emissionStrength = 1.0f;
    }
    // Do NOT auto-treat white materials as emission - only if they explicitly have emission properties
    else {
        material.emissionStrength = 0.0f;
        Logger::info("  Emission Strength (DEFAULT): 0.0");
    }
    
    // Log emission summary
    Logger::info("  --> Material name: " + matNameStr + ", emission strength: " + std::to_string(material.emissionStrength) + ", emission: (" + std::to_string(material.emission.r) + "," + std::to_string(material.emission.g) + "," + std::to_string(material.emission.b) + ")");
    Logger::info("=== Material loaded ===\n");
    
    return material;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const aiScene* scene, int materialIndex) {
    std::vector<Texture> textures;

    unsigned int count = mat->GetTextureCount(type);
    Logger::info("  === loadMaterialTextures: type=" + typeName + ", ASSIMP type=" + std::to_string((int)type) + ", count=" + std::to_string(count));

    // Get material name to detect if it should tile
    aiString matName;
    mat->Get(AI_MATKEY_NAME, matName);
    std::string matNameStr = matName.C_Str();
    
    for (unsigned int i = 0; i < count; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texPath = str.C_Str();
        Logger::info("    Texture " + std::to_string(i) + ": path='" + texPath + "'");

        // Get UV transform from glTF if present
        glm::vec2 uvScale = glm::vec2(1.0f, 1.0f);
        glm::vec2 uvOffset = glm::vec2(0.0f, 0.0f);
        
        if (materialIndex >= 0) {
            std::string key = std::to_string(materialIndex) + "," + std::to_string(i);
            auto it = m_textureTransforms.find(key);
            if (it != m_textureTransforms.end()) {
                uvScale = it->second.scale;
                uvOffset = it->second.offset;
                Logger::info("    -> Using glTF texture transform: scale(" + std::to_string(uvScale.x) + "," + 
                            std::to_string(uvScale.y) + ") offset(" + std::to_string(uvOffset.x) + "," + 
                            std::to_string(uvOffset.y) + ")");
            } else {
                // No glTF texture transform present; respect the mesh's exported UVs instead of applying heuristics.
                // Leaving uvScale at (1,1) so vertex UVs determine tiling/repetition.
                Logger::info("    -> No glTF texture transform - using default (1,1)");
            }
        }

        unsigned int texID = 0;
        std::string key = m_directory + "/" + texPath;

        // Embedded texture (starts with '*')
        if (!texPath.empty() && texPath[0] == '*') {
            int texIndex = atoi(texPath.c_str() + 1);
            Logger::info("    -> Embedded texture index: " + std::to_string(texIndex));
            if (scene && scene->mTextures && texIndex < (int)scene->mNumTextures) {
                aiTexture* atex = scene->mTextures[texIndex];
                        if (!atex->pcData) {
                            Logger::error("    -> Embedded texture data pointer is null for index: " + std::to_string(texIndex));
                        }

                        if (atex->mHeight == 0) {
                    // Compressed image in memory (PNG/JPEG)
                            const unsigned char* data = reinterpret_cast<const unsigned char*>(atex->pcData);
                            int size = atex->mWidth; // size in bytes
                            // Sanity checks to avoid passing bogus sizes into stbi_load_from_memory
                            if (size <= 0) {
                                Logger::error("    -> Embedded compressed texture reported size <= 0 for index " + std::to_string(texIndex) + " - skipping");
                                continue;
                            }
                            if (size > 200 * 1024 * 1024) {
                                Logger::error("    -> Embedded compressed texture size looks suspicious (>200MB) for index " + std::to_string(texIndex) + " - skipping");
                                continue;
                            }
                        // Use the full model path as part of the key to avoid collisions
                        key = m_modelPath + "#embedded_" + std::to_string(texIndex);
                    // Choose sRGB for base color / albedo textures, otherwise handled by caller
                    bool useSRGB = (typeName == "albedo");
                    Logger::info("    -> Calling loadTextureFromMemory: key='" + key + "', srgb=" + std::to_string(useSRGB));
                    texID = m_textureManager.loadTextureFromMemory(key, data, size, useSRGB);
                } else {
                    // Uncompressed RGBA data
                    int width = atex->mWidth;
                    int height = atex->mHeight;
                    if (width <= 0 || height <= 0) {
                        Logger::error("    -> Embedded raw texture has invalid dimensions (" + std::to_string(width) + "x" + std::to_string(height) + ") for index " + std::to_string(texIndex) + " - skipping");
                        continue;
                    }
                    const unsigned char* data = reinterpret_cast<const unsigned char*>(atex->pcData);
                    // Create GL texture directly
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    glGenTextures(1, &texID);
                    glBindTexture(GL_TEXTURE_2D, texID);
                    // Use explicit sized internal formats: sRGB8 alpha for albedo, RGBA8 otherwise
                    GLenum internalFmt = (typeName == "albedo") ? GL_SRGB8_ALPHA8 : GL_RGBA8;
                    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    // Raw embedded image: include model path so keys are unique
                    key = m_modelPath + "#embedded_raw_" + std::to_string(texIndex);
                    m_texturesLoaded[key] = texID;
                    Logger::info("  Created texture from embedded raw data: " + key);
                }
            }
        }
        else {
            // External file - try to resolve relative to model directory
            std::string fullPath = texPath;
            if (texPath.find_first_of("/:\\") == std::string::npos) {
                fullPath = m_directory + "/" + texPath;
            }
            texID = m_textureManager.loadTexture(fullPath);
            key = fullPath;
        }

        if (texID != 0) {
            Texture texture;
            texture.id = texID;
            // Map our semantic name to shader-friendly names
            if (typeName == "albedo") texture.type = "albedo";
            else if (typeName == "normal") texture.type = "normal";
            else if (typeName == "metallicRoughness") texture.type = "metallicRoughness";
            else if (typeName == "ao") texture.type = "ao";
            else texture.type = typeName;

            texture.path = key;
            
            // Apply UV transform (from glTF parse). Also set which UV set to use
            texture.uvScale = uvScale;
            texture.uvOffset = uvOffset;
            // If we parsed a transform for this material,texture slot, it may contain texCoord
            std::string mapKey = std::to_string(materialIndex) + "," + std::to_string(i);
            auto it = m_textureTransforms.find(mapKey);
            if (it != m_textureTransforms.end()) {
                texture.uvSet = it->second.texCoord;
            } else {
                texture.uvSet = 0;
            }
            
            textures.push_back(texture);
            Logger::info("  ✓ Loaded texture: " + key + " as " + texture.type + " (ID=" + std::to_string(texID) + 
                        ") uvScale=(" + std::to_string(texture.uvScale.x) + "," + std::to_string(texture.uvScale.y) + 
                        ") uvOffset=(" + std::to_string(texture.uvOffset.x) + "," + std::to_string(texture.uvOffset.y) + 
                        ") uvSet=" + std::to_string(texture.uvSet));
        } else {
            Logger::info("  ✗ Failed to load texture: " + texPath);
        }
    }

    return textures;
}

void Model::draw(unsigned int shaderProgram) {
    Logger::info("Model::draw called - rendering " + std::to_string(m_meshes.size()) + " meshes, transform scale: " + 
                 std::to_string(m_scale.x) + "," + std::to_string(m_scale.y) + "," + std::to_string(m_scale.z));
    for (auto& mesh : m_meshes) {
        if (mesh) {
            Logger::info("  Mesh draw: indices=" + std::to_string(mesh->getIndexCount()));
            mesh->draw(shaderProgram, m_transform);
        }
    }
}

void Model::setTransform(const glm::mat4& transform) {
    m_transform = transform;
}

void Model::setPosition(const glm::vec3& position) {
    m_position = position;
    updateTransform();
}

void Model::setRotation(const glm::vec3& rotation) {
    m_rotation = rotation;
    updateTransform();
}

void Model::setScale(const glm::vec3& scale) {
    m_scale = scale;
    updateTransform();
}

void Model::updateTransform() {
    m_transform = glm::mat4(1.0f);
    m_transform = glm::translate(m_transform, m_position);
    m_transform = glm::rotate(m_transform, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    m_transform = glm::rotate(m_transform, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    m_transform = glm::rotate(m_transform, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    m_transform = glm::scale(m_transform, m_scale);
}

void Model::drawForShadow(unsigned int shadowShader, const glm::mat4& modelMatrix) {
    glUseProgram(shadowShader);
    glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
    
    for (auto& mesh : m_meshes) {
        mesh->drawForShadow(shadowShader, modelMatrix);
    }
}

// Extract JSON chunk from .glb file (binary glTF format)
std::string Model::extractGltfJson(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Failed to open glTF file for JSON extraction: " + path);
        return "";
    }
    
    // .glb file format:
    // 0-3: magic number (0x46546C67 = "glTF")
    // 4-7: version (2 for glTF 2.0)
    // 8-11: file length
    // 12-15: chunk length
    // 16-19: chunk type (0x4E4F534A = "JSON")
    // 20+: JSON data
    
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), 4);
    
    if (magic != 0x46546C67) {
        // Not a .glb file, might be plain .gltf
        return "";
    }
    
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), 4);
    
    uint32_t fileLength;
    file.read(reinterpret_cast<char*>(&fileLength), 4);
    
    uint32_t chunkLength;
    file.read(reinterpret_cast<char*>(&chunkLength), 4);
    
    uint32_t chunkType;
    file.read(reinterpret_cast<char*>(&chunkType), 4);
    
    
    // chunkType should be 0x4E4F534A ("JSON")
    if (chunkType != 0x4E4F534A) {
        Logger::error("First chunk is not JSON type in glTF file (chunkType=0x" + std::to_string(chunkType) + ")");
        return "";
    }
    
    // Read JSON data
    std::string json(chunkLength, '\0');
    file.read(&json[0], chunkLength);
    
    Logger::info("Successfully extracted glTF JSON, size: " + std::to_string(chunkLength) + " bytes");
    return json;
}

// Parse texture transforms from glTF JSON for a specific material
void Model::parseGltfTextures(const std::string& jsonStr, int materialIndex) {
    if (jsonStr.empty() || materialIndex < 0) return;
    
    // Find the material in the JSON
    std::string matSearchStr = "\"materials\"";
    size_t matsPos = jsonStr.find(matSearchStr);
    if (matsPos == std::string::npos) {
        return;
    }
    
    // Find opening of materials array
    size_t arrayStart = jsonStr.find('[', matsPos);
    if (arrayStart == std::string::npos) {
        return;
    }
    
    size_t currentPos = arrayStart + 1;
    int braceDepth = 0;
    std::string currentMaterial;
    int matCount = 0;
    bool foundTarget = false;
    
    // Extract the specific material object
    while (currentPos < jsonStr.length()) {
        char c = jsonStr[currentPos];
        
        if (c == '{') {
            if (braceDepth == 0 && matCount == materialIndex) {
                // Start of our material
                currentMaterial.clear();
                foundTarget = true;
            }
            braceDepth++;
            if (foundTarget) currentMaterial += c;
        }
        else if (c == '}') {
            if (foundTarget) {
                currentMaterial += c;
            }
            braceDepth--;
            if (foundTarget && braceDepth == 0) {
                // End of our material
                break;
            }
            // If brace depth returns to 0 and we haven't found target yet, increment material count
            if (braceDepth == 0 && !foundTarget) {
                matCount++;
            }
        }
        else if (foundTarget && braceDepth > 0) {
            currentMaterial += c;
        }
        
        currentPos++;
    }
    
    if (currentMaterial.empty()) {
        Logger::info("Material " + std::to_string(materialIndex) + " not found in glTF JSON - using fallback");
        return;
    }

    // Note: don't require the presence of KHR_texture_transform to continue parsing.
    // We still want to detect `texCoord` fields even when no KHR extension is present
    // so we can honor explicit texCoord indices authored by the exporter.
    bool hasKHR = (currentMaterial.find("KHR_texture_transform") != std::string::npos);
    if (!hasKHR) {
        Logger::info("Material " + std::to_string(materialIndex) + " has no KHR_texture_transform extension - will still parse texCoord indices if present");
    }
    
    // Look for texture indices and their transforms
    std::string texPatterns[] = {
        "\"baseColorTexture\"",
        "\"normalTexture\"",
        "\"metallicRoughnessTexture\"",
        "\"occlusionTexture\""
    };
    
    int textureIndex = 0;
    for (const auto& pattern : texPatterns) {
        size_t pos = currentMaterial.find(pattern);
        if (pos == std::string::npos) continue;
        
        // Find the texture block: {index:N, extensions:{KHR_texture_transform:{scale:[],offset:[]}}}
        size_t blockStart = currentMaterial.find('{', pos);
        if (blockStart == std::string::npos) continue;
        
        // Find the end of the block. Use a simple brace matcher to handle nested objects.
        size_t blockEnd = std::string::npos;
        int depth = 0;
        for (size_t p = blockStart; p < currentMaterial.size(); ++p) {
            if (currentMaterial[p] == '{') depth++;
            else if (currentMaterial[p] == '}') {
                depth--;
                if (depth == 0) {
                    blockEnd = p;
                    break;
                }
            }
        }
        if (blockEnd == std::string::npos) continue;
        if (blockEnd == std::string::npos) continue;
        
        std::string texBlock = currentMaterial.substr(blockStart, blockEnd - blockStart + 1);
        
        // Parse index and extensions
        // Parse transform and texCoord. parseTextureTransform will extract texCoord
        // even if there is no KHR extension present (it now checks texCoord unconditionally).
        TextureTransform transform = parseTextureTransform(texBlock);
        
        // Store with key "material,textureIndex"
        std::string key = std::to_string(materialIndex) + "," + std::to_string(textureIndex);
        m_textureTransforms[key] = transform;
        
        Logger::info("  Material " + std::to_string(materialIndex) + " texture " + std::to_string(textureIndex) + 
                    ": scale(" + std::to_string(transform.scale.x) + "," + std::to_string(transform.scale.y) + 
                    ") offset(" + std::to_string(transform.offset.x) + "," + std::to_string(transform.offset.y) + ")");
        
        textureIndex++;
    }
}

// Parse a single texture's transform data
TextureTransform Model::parseTextureTransform(const std::string& textureSection) {
    TextureTransform transform;
    // Parse texCoord (UV set) if present (e.g. "texCoord":1) - do this unconditionally
    size_t texCoordPos = textureSection.find("\"texCoord\"");
    if (texCoordPos != std::string::npos) {
        size_t colon = textureSection.find(':', texCoordPos);
        if (colon != std::string::npos) {
            size_t comma = textureSection.find_first_of(",}\n", colon+1);
            std::string val = textureSection.substr(colon+1, (comma==std::string::npos?textureSection.size():comma)-(colon+1));
            try {
                transform.texCoord = std::stoi(val);
            } catch(...) {
                // ignore
            }
        }
    }

    // Look for KHR_texture_transform in extensions (if present) and parse scale/offset
    size_t extStart = textureSection.find("\"extensions\"");
    if (extStart == std::string::npos) {
        // No extensions block — keep default scale/offset
        return transform;
    }

    size_t khrStart = textureSection.find("\"KHR_texture_transform\"", extStart);
    if (khrStart == std::string::npos) {
        // No KHR extension — keep defaults
        return transform;
    }

    // Find scale array
    size_t scaleStart = textureSection.find("\"scale\"", khrStart);
    if (scaleStart != std::string::npos) {
        size_t arrayStart = textureSection.find('[', scaleStart);
        size_t arrayEnd = textureSection.find(']', arrayStart);
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string scaleStr = textureSection.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
            // Parse two floats
            size_t comma = scaleStr.find(',');
            if (comma != std::string::npos) {
                try {
                    transform.scale.x = std::stof(scaleStr.substr(0, comma));
                    transform.scale.y = std::stof(scaleStr.substr(comma + 1));
                } catch (...) {
                    // Parse error, use defaults
                }
            }
        }
    }

    // Find offset array
    size_t offsetStart = textureSection.find("\"offset\"", khrStart);
    if (offsetStart != std::string::npos) {
        size_t arrayStart = textureSection.find('[', offsetStart);
        size_t arrayEnd = textureSection.find(']', arrayStart);
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string offsetStr = textureSection.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
            // Parse two floats
            size_t comma = offsetStr.find(',');
            if (comma != std::string::npos) {
                try {
                    transform.offset.x = std::stof(offsetStr.substr(0, comma));
                    transform.offset.y = std::stof(offsetStr.substr(comma + 1));
                } catch (...) {
                    // Parse error, use defaults
                }
            }
        }
    }

    return transform;
}

#ifdef USE_NLOHMANN
// JSON-based parsing using nlohmann::json. This is more robust and will
// extract texCoord, scale and offset from standard glTF fields.
void Model::parseGltfTexturesJson(const std::string& jsonStr, int materialIndex) {
    try {
        auto root = json::parse(jsonStr);
        if (!root.contains("materials")) return;
        auto materials = root["materials"];
        if (!materials.is_array()) return;
        if (materialIndex < 0 || materialIndex >= (int)materials.size()) return;
        auto mat = materials[materialIndex];

        // Patterns in the same order as the old parser: baseColor, normal, metallicRoughness, occlusion
        int textureIndex = 0;

        // Helper lambda to parse a texture object and store transform
        auto handleTextureNode = [&](const json& texNode) {
            if (texNode.is_null()) return;
            TextureTransform t;
            // texCoord (optional)
            if (texNode.contains("texCoord")) {
                t.texCoord = texNode["texCoord"].get<int>();
            }
            // extensions -> KHR_texture_transform
            if (texNode.contains("extensions") && texNode["extensions"].contains("KHR_texture_transform")) {
                auto khr = texNode["extensions"]["KHR_texture_transform"];
                if (khr.contains("scale") && khr["scale"].is_array() && khr["scale"].size() >= 2) {
                    t.scale.x = khr["scale"][0].get<float>();
                    t.scale.y = khr["scale"][1].get<float>();
                }
                if (khr.contains("offset") && khr["offset"].is_array() && khr["offset"].size() >= 2) {
                    t.offset.x = khr["offset"][0].get<float>();
                    t.offset.y = khr["offset"][1].get<float>();
                }
            }

            std::string key = std::to_string(materialIndex) + "," + std::to_string(textureIndex);
            m_textureTransforms[key] = t;
            Logger::info("  Material " + std::to_string(materialIndex) + " texture " + std::to_string(textureIndex) + ": scale(" + std::to_string(t.scale.x) + "," + std::to_string(t.scale.y) + ") offset(" + std::to_string(t.offset.x) + "," + std::to_string(t.offset.y) + ") texCoord=" + std::to_string(t.texCoord));
            textureIndex++;
        };

        // baseColorTexture
        if (mat.contains("pbrMetallicRoughness") && mat["pbrMetallicRoughness"].contains("baseColorTexture")) {
            handleTextureNode(mat["pbrMetallicRoughness"]["baseColorTexture"]);
        }
        // normalTexture
        if (mat.contains("normalTexture")) {
            handleTextureNode(mat["normalTexture"]);
        }
        // metallicRoughnessTexture
        if (mat.contains("pbrMetallicRoughness") && mat["pbrMetallicRoughness"].contains("metallicRoughnessTexture")) {
            handleTextureNode(mat["pbrMetallicRoughness"]["metallicRoughnessTexture"]);
        }
        // occlusionTexture
        if (mat.contains("occlusionTexture")) {
            handleTextureNode(mat["occlusionTexture"]);
        }

    } catch (const std::exception& ex) {
        Logger::error(std::string("Failed to parse glTF JSON with nlohmann::json: ") + ex.what());
    }
}
#endif