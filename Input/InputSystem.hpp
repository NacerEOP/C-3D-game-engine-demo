#pragma once
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include "../Graphics/Camera.hpp"

class InputSystem {
private:
    bool m_keys[sf::Keyboard::KeyCount] = {false};
    glm::vec2 m_mouseDelta;
    glm::vec2 m_lastMousePos;
    bool m_mouseGrabbed;
    sf::Window* m_window;
    bool m_isRunningKeyPressed;

public:
    InputSystem();
    
    void update();
    void handleEvent(const sf::Event& event);
    void processCameraInput(Camera& camera, float deltaTime);
    
    bool isKeyPressed(sf::Keyboard::Key key) const;
    glm::vec2 getMouseDelta() const { return m_mouseDelta; }
    void setMouseGrabbed(bool grabbed) { m_mouseGrabbed = grabbed; }
    void setWindow(sf::Window* window) { m_window = window; }
};