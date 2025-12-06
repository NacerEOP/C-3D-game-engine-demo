#include "Time.hpp"

Time::Time() 
    : m_deltaTime(0.0f)
    , m_totalTime(0.0f) {
    m_lastTime = std::chrono::high_resolution_clock::now();
}

void Time::update() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastTime);
    
    m_deltaTime = duration.count() / 1000000.0f; // Convert to seconds
    m_totalTime += m_deltaTime;
    m_lastTime = currentTime;
}

float Time::getTimeSinceEpoch() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0f;
}