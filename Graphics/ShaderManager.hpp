#pragma once
#include <unordered_map>
#include <string>
#include "../include/glad/glad.h"

class ShaderManager {
private:
    std::unordered_map<std::string, unsigned int> m_shaders;
    
public:
    ShaderManager() = default;
    ~ShaderManager();
    
    unsigned int loadShader(const std::string& name, const char* vertexSrc, const char* fragmentSrc);
    unsigned int loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    unsigned int getShader(const std::string& name);
    void cleanup();
    
private:
    unsigned int compileShader(const char* vertexSrc, const char* fragmentSrc);
    bool checkCompileErrors(unsigned int shader, const std::string& type);
    std::string readFile(const std::string& filepath);
};