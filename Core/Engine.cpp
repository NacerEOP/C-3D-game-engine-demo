#include "Engine.hpp"
#if defined(__has_include)
# if __has_include(<assimp/DefaultLogger.hpp>)
#  include <assimp/DefaultLogger.hpp>
#  define ASSIMP_DEFAULTLOGGER_AVAILABLE 1
# endif
#endif
#include "../Utils/Logger.hpp"
#include <iostream>

Engine::Engine() 
    : m_renderer(Renderer::getInstance())
    , m_running(false) {
}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize() {
    Logger::info("=== Starting 3D Game Engine ===");
    
    if (!m_app.initialize()) {
        Logger::error("Failed to initialize application!");
        return false;
    }
    // Initialize scene
    m_scene.initialize();

    if (!m_renderer.initialize()) {
        Logger::error("Failed to initialize renderer!");
        return false;
    }
    
    
    
    // NOW create scene objects after OpenGL is ready
    m_scene.loadModels();
    
    // Connect scene to renderer
    m_renderer.setScene(&m_scene);
    
    m_running = true;
    Logger::info("Engine initialized successfully!");
    return true;
}

void Engine::run() {
    mainLoop();
    Logger::info("Engine shutdown complete.");
}

void Engine::mainLoop() {
    while (m_running && m_app.isRunning()) {
        m_time.update();
        m_app.handleEvents();
        
        // Update game logic with proper delta time
        float deltaTime = m_time.getDeltaTime();
        
        // Process input with actual frame delta time
        // Use the InputSystem directly from Application
        m_app.getInputSystem().processCameraInput(m_app.getCamera(), deltaTime);
        
        // Update scene
        m_scene.update(deltaTime);
        
        // Render
        m_renderer.beginFrame();
        m_renderer.renderScene(m_app.getCamera()); // Use camera from Application
        m_renderer.endFrame();
        
        m_app.display();
        
        m_running = m_app.isRunning();
    }
}

void Engine::shutdown() {
    m_renderer.shutdown();
    // Try to explicitly kill Assimp's default logger before program exit.
    // Some versions of the assimp DLL hold static objects that flush to
    // iostreams during DLL unload; forcing logger cleanup here reduces
    // chances of hitting destructors ordering issues on process shutdown.
#ifdef ASSIMP_DEFAULTLOGGER_AVAILABLE
    try {
        Assimp::DefaultLogger::kill();
    } catch (...) {
        Logger::warn("Exception while trying to kill Assimp DefaultLogger");
    }
#endif
    m_running = false;
}