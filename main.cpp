#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

#include "Core/Engine.hpp"
#include "Utils/Logger.hpp"

int main() {
    Logger::info("=== Starting 3D Game Engine ===");
    
    Engine engine;
    
    if (engine.initialize()) {
        engine.run();
        Logger::info("Engine shutdown complete.");
    } else {
        Logger::error("Failed to initialize engine!");
        return -1;
    }
    
    return 0;
}