#include "Camera.hpp"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include <iostream>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
    : m_position(position), m_worldUp(up), m_yaw(yaw), m_pitch(pitch), 
      m_movementSpeed(5.0f), m_mouseSensitivity(0.1f), m_eyeHeight(1.7f) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::processKeyboard(int direction, float deltaTime, bool isRunning) {
    float velocity = m_movementSpeed * deltaTime;
    if (isRunning) velocity *= 2.0f; // Shift to run - doubles speed

    

    // First-person movement: only move in XZ plane (ground plane)
    glm::vec3 movement(0.0f);
    
    if (direction == 0) // Forward - only use X and Z components
        movement += glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));
    if (direction == 1) // Backward
        movement -= glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));
    if (direction == 2) // Left
        movement -= m_right;
    if (direction == 3) // Right
        movement += m_right;

    // Apply movement (only X and Z change, Y stays at eye height)
    if (glm::length(movement) > 0.0f) {
        m_position += movement * velocity;
        m_position.y = m_eyeHeight; // Always maintain eye height
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    // Prevent looking too far up or down
    if (constrainPitch) {
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    // Calculate the new front vector
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    
    m_front = glm::normalize(front);
    
    // Also re-calculate the right and up vectors
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));  
    m_up = glm::normalize(glm::cross(m_right, m_front));
}