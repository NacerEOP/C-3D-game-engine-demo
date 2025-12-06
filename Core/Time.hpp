#pragma once
#include <chrono>

class Time {
private:
    std::chrono::high_resolution_clock::time_point m_lastTime;
    float m_deltaTime;
    float m_totalTime;

public:
    Time();
    
    void update();
    float getDeltaTime() const { return m_deltaTime; }
    float getTotalTime() const { return m_totalTime; }
    
    static float getTimeSinceEpoch();
};