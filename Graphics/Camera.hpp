#pragma once
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"

class Camera {
private:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;
    
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_eyeHeight;  // First-person eye height

public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 1.7f, 0.0f),  // Start at eye height
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 getViewMatrix() const;
    void processKeyboard(int direction, float deltaTime, bool isRunning);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // Getters
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getFront() const { return m_front; }

private:
    void updateCameraVectors();
};