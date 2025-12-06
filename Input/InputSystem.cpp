#include "InputSystem.hpp"
#include <iostream>

InputSystem::InputSystem() 
    : m_mouseGrabbed(false)
    , m_window(nullptr)
    , m_isRunningKeyPressed(false) {
    m_mouseDelta = glm::vec2(0.0f);
    m_lastMousePos = glm::vec2(0.0f);
}

void InputSystem::update() {
    if (m_mouseGrabbed && m_window) {
        sf::Vector2i currentMousePos = sf::Mouse::getPosition(*m_window);
        glm::vec2 currentPos(currentMousePos.x, currentMousePos.y);
        
        // Get window center
        sf::Vector2u windowSize = m_window->getSize();
        glm::vec2 windowCenter(windowSize.x / 2, windowSize.y / 2);
        
        if (m_lastMousePos != glm::vec2(0.0f)) {
            m_mouseDelta = currentPos - m_lastMousePos;
        }
        
        m_lastMousePos = currentPos;
        
        // Reset mouse to center of window to prevent it from getting stuck
        sf::Mouse::setPosition(sf::Vector2i(windowCenter.x, windowCenter.y), *m_window);
        m_lastMousePos = windowCenter; // Update last position to center
    } else {
        m_mouseDelta = glm::vec2(0.0f);
    }
}

void InputSystem::handleEvent(const sf::Event& event) {
    if (auto keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        m_keys[static_cast<int>(keyEvent->code)] = true;

        if (keyEvent->code == sf::Keyboard::Key::LShift) {
            m_isRunningKeyPressed = true;
        }
    }
    else if (auto keyEvent = event.getIf<sf::Event::KeyReleased>()) {
        m_keys[static_cast<int>(keyEvent->code)] = false;

        if (keyEvent->code == sf::Keyboard::Key::LShift) {
            m_isRunningKeyPressed = false;
        }
    }
}

void InputSystem::processCameraInput(Camera& camera, float deltaTime) {
    bool wPressed = isKeyPressed(sf::Keyboard::Key::W);
    bool aPressed = isKeyPressed(sf::Keyboard::Key::A);
    bool sPressed = isKeyPressed(sf::Keyboard::Key::S);
    bool dPressed = isKeyPressed(sf::Keyboard::Key::D);
    
    // Keyboard movement
    if (wPressed)
        camera.processKeyboard(0, deltaTime, m_isRunningKeyPressed);
    if (sPressed)
        camera.processKeyboard(1, deltaTime, m_isRunningKeyPressed);
    if (aPressed)
        camera.processKeyboard(2, deltaTime, m_isRunningKeyPressed);
    if (dPressed)
        camera.processKeyboard(3, deltaTime, m_isRunningKeyPressed);
    
    // Mouse look
    if (m_mouseDelta != glm::vec2(0.0f)) {
        camera.processMouseMovement(m_mouseDelta.x, -m_mouseDelta.y);
        m_mouseDelta = glm::vec2(0.0f); // Reset after processing
    }
}

bool InputSystem::isKeyPressed(sf::Keyboard::Key key) const {
    return m_keys[static_cast<int>(key)];
}