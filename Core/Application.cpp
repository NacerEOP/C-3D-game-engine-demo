#include "Application.hpp"
#include "../Graphics/Renderer.hpp"
#include "../Utils/Logger.hpp"
#include <iostream>

Application::Application() 
    : m_running(false) 
    , m_inputSystem(std::make_unique<InputSystem>()) {
}

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    m_window = std::make_unique<sf::Window>(
        sf::VideoMode({1280, 720}), "3D Game Engine - PBR + Shadows", sf::Style::Default
    );
    if (!m_window || !m_window->isOpen()) {
        Logger::error("Failed to create SFML window or window not open");
        return false;
    }
    
    // Hide cursor and capture mouse
    m_window->setMouseCursorGrabbed(true);
    m_window->setMouseCursorVisible(false);
    m_window->setVerticalSyncEnabled(true);
    
    m_inputSystem->setMouseGrabbed(true);
    m_inputSystem->setWindow(m_window.get());
    
    m_running = true;
    return true;
}

void Application::run() {
    // This is now handled by Engine class
}

void Application::handleEvents() {
    for (auto event = m_window->pollEvent(); event.has_value(); event = m_window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            Logger::info("Application received Close event");
            m_running = false;
        }
        if (event->is<sf::Event::Resized>()) {
            Logger::info("Window resized event received");
        }
        m_inputSystem->handleEvent(*event);
    }
    m_inputSystem->update();
}

void Application::shutdown() {
    m_running = false;
    if (m_window) {
        m_window->close();
    }
}