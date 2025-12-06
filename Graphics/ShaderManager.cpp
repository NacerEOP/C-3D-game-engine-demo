#include "ShaderManager.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "../Utils/Logger.hpp"

ShaderManager::~ShaderManager() {
    cleanup();
}

unsigned int ShaderManager::loadShader(const std::string& name, const char* vertexSrc, const char* fragmentSrc) {
    unsigned int program = compileShader(vertexSrc, fragmentSrc);
    if (program != 0) {
        m_shaders[name] = program;
    }
    return program;
}

unsigned int ShaderManager::loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);
    
    if (vertexCode.empty() || fragmentCode.empty()) {
        Logger::error(std::string("Failed to load shader files: ") + vertexPath + " or " + fragmentPath);
        return 0;
    }
    
    return loadShader(name, vertexCode.c_str(), fragmentCode.c_str());
}

unsigned int ShaderManager::getShader(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }
    return 0;
}

void ShaderManager::cleanup() {
    for (auto& shader : m_shaders) {
        glDeleteProgram(shader.second);
    }
    m_shaders.clear();
}

unsigned int ShaderManager::compileShader(const char* vertexSrc, const char* fragmentSrc) {
    // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, NULL);
    glCompileShader(vertexShader);
    if (!checkCompileErrors(vertexShader, "VERTEX")) {
        return 0;
    }

    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, NULL);
    glCompileShader(fragmentShader);
    if (!checkCompileErrors(fragmentShader, "FRAGMENT")) {
        glDeleteShader(vertexShader);
        return 0;
    }

    // Shader program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    if (!checkCompileErrors(program, "PROGRAM")) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

bool ShaderManager::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            Logger::error(std::string("SHADER_COMPILATION_ERROR of type: ") + type + "\n" + infoLog);
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) Logger::error(std::string("OpenGL error while compiling shader (") + type + "): " + std::to_string(err));
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            Logger::error(std::string("PROGRAM_LINKING_ERROR of type: ") + type + "\n" + infoLog);
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) Logger::error(std::string("OpenGL error while linking program: ") + std::to_string(err));
            return false;
        }
    }
    return true;
}

std::string ShaderManager::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::error(std::string("Failed to open file: ") + filepath);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}