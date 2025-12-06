#pragma once
#include "Application.hpp"
#include "../Graphics/Renderer.hpp"
#include "../Input/InputSystem.hpp"
#include "../Scene/Scene.hpp"
#include "Time.hpp"

class Engine {
private:
    Application m_app;
    Renderer& m_renderer;
    Scene m_scene;
    Time m_time;
    bool m_running;

public:
    Engine();
    ~Engine();
    
    bool initialize();
    void run();
    void shutdown();
    
private:
    void mainLoop();
};