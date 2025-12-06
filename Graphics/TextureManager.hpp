#pragma once
#include <unordered_map>
#include <string>
#include "../include/glad/glad.h"

class TextureManager {
private:
    std::unordered_map<std::string, unsigned int> m_textures;

public:
    TextureManager() = default;
    ~TextureManager();
    
    unsigned int loadTexture(const std::string& path);
    // 'srgb' indicates whether the image should be uploaded with an sRGB internal format
    unsigned int loadTextureFromMemory(const std::string& key, const unsigned char* data, int size, bool srgb = true);
    unsigned int getTexture(const std::string& path);
    // Return the first available texture ID (useful for quick debug visualization)
    unsigned int getFirstTexture();
    void cleanup();
    
private:
    unsigned int textureFromFile(const std::string& path);
};