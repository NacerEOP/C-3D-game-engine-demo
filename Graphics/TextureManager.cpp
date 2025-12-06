#include "TextureManager.hpp"
#include "../Utils/Logger.hpp"
#include <iostream>
#include "../include/stb_image.h"  // Always include; stb_image_impl.cpp provides the implementation

TextureManager::~TextureManager() {
    cleanup();
}

unsigned int TextureManager::loadTexture(const std::string& path) {
    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        return it->second;
    }

    unsigned int textureID = textureFromFile(path);
    if (textureID != 0) {
        m_textures[path] = textureID;
        Logger::info("Loaded texture: " + path);
    }

    return textureID;
}

unsigned int TextureManager::loadTextureFromMemory(const std::string& key, const unsigned char* data, int size, bool srgb) {
    auto it = m_textures.find(key);
    if (it != m_textures.end()) {
        Logger::info("[TextureManager] Texture already loaded (cache hit): " + key + " -> ID " + std::to_string(it->second));
        return it->second;
    }

    Logger::info("[TextureManager] loadTextureFromMemory called with key: " + key + " (size: " + std::to_string(size) + " bytes, srgb=" + std::to_string(srgb) + ")");

    // Additional diagnostics for problematic inputs
    if (data == nullptr) {
        Logger::error(std::string("[TextureManager] ERROR: null data pointer for key: ") + key);
    }
    if (size <= 0) {
        Logger::error(std::string("[TextureManager] ERROR: invalid size (<=0) passed for key: ") + key + " -> " + std::to_string(size));
    }
    
    int width, height, channels;
    unsigned char* img = stbi_load_from_memory(data, size, &width, &height, &channels, 4);
    if (!img) {
        Logger::error("[TextureManager] stbi_load_from_memory failed for key: " + key);
        Logger::info("[TextureManager] Falling back to placeholder texture");
        return textureFromFile("");
    }

    Logger::info("[TextureManager] Successfully decoded image: " + std::to_string(width) + "x" + std::to_string(height) + ", channels=" + std::to_string(channels));
    
    GLenum format = GL_RGBA;  // We requested 4 channels
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    Logger::info("[TextureManager] Created GL texture ID: " + std::to_string(textureID));
    // Use sRGB internal format for color (albedo) maps when requested, otherwise use linear RGBA
    // Use explicit sized internal formats. For albedo (srgb==true) use sRGB8 alpha8
    GLenum internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(img);
    // Log OpenGL errors that might happen during texture upload
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::error(std::string("[TextureManager] OpenGL error after uploading texture '") + key + "': " + std::to_string(err));
    }
    m_textures[key] = textureID;
    Logger::info("[TextureManager] ✓ Stored in map: " + key + " -> GL texture ID " + std::to_string(textureID) + ". Map size now: " + std::to_string(m_textures.size()));
    return textureID;
}

unsigned int TextureManager::getTexture(const std::string& path) {
    auto it = m_textures.find(path);
    return (it != m_textures.end()) ? it->second : 0;
}

void TextureManager::cleanup() {
    for (auto& texture : m_textures) {
        glDeleteTextures(1, &texture.second);
    }
    m_textures.clear();
}

unsigned int TextureManager::getFirstTexture() {
    Logger::info("[TextureManager] getFirstTexture called. Map size: " + std::to_string(m_textures.size()));
    if (!m_textures.empty()) {
        for (const auto& kv : m_textures) {
            Logger::info("[TextureManager]   Key: " + kv.first + " -> ID: " + std::to_string(kv.second));
            // Return the first albedo texture found (for debug visualization)
            if (kv.first.find("embedded_0") != std::string::npos) {
                return kv.second;
            }
        }
        return m_textures.begin()->second;
    }
    Logger::info("[TextureManager] No textures in map!");
    return 0;
}

unsigned int TextureManager::textureFromFile(const std::string& path) {
    // If path is empty or stb not available, create a simple placeholder
    unsigned int textureID;
    glGenTextures(1, &textureID);

    unsigned char placeholder[] = {
        255, 0, 255,   255, 255, 255,
        255, 255, 255,   255, 0, 255
    };

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, placeholder);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}