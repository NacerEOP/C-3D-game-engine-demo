#pragma once
#include <SFML/Window.hpp>
#include <memory>
#include "../Graphics/Camera.hpp"
#include "../Input/InputSystem.hpp"

class Application {
private:
    std::unique_ptr<sf::Window> m_window;
    std::unique_ptr<InputSystem> m_inputSystem;
    Camera m_camera;
    bool m_running;

public:
    Application();
    ~Application();
    
    bool initialize();
    void run();
    void shutdown();
    
    void handleEvents();
    void display() { m_window->display(); }
    
    bool isRunning() const { return m_running; }
    InputSystem& getInputSystem() { return *m_inputSystem; }
    Camera& getCamera() { return m_camera; }
};