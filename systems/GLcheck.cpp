#include "GLcheck.hpp"
#include <SFML/Window.hpp>
#include "../include/glad/glad.h"
#include <iostream>
#include "../Utils/Logger.hpp"

bool initializeOpenGL()
{
    // Create window with OpenGL context
    sf::Window window(sf::VideoMode({800, 600}), "3D Game Engine");
    
    // Initialize GLAD
    if (!gladLoadGL()) {
        Logger::error("Failed to initialize GLAD!");
        return false;
    }

    Logger::info("=== OpenGL Initialized ===");
    Logger::info(std::string("Version: ") + reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    Logger::info(std::string("Vendor: ") + reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    Logger::info(std::string("Renderer: ") + reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Logger::info(std::string("GLAD loaded: ") + std::to_string(GLVersion.major) + "." + std::to_string(GLVersion.minor));
    
    // Basic OpenGL setup
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    
    return true;
}

void runEngine()
{
    sf::Window window(sf::VideoMode({800, 600}), "3D Game Engine");
    
    // Main engine loop
    while (window.isOpen()) {
        // Handle events
        for (auto event = window.pollEvent(); event.has_value(); event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        
        // Render
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Your 3D rendering will go here later
        
        window.display();
    }
}