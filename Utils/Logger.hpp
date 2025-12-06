#pragma once
#include <iostream>
#include <string>

class Logger {
public:
    static void info(const std::string& message) {
        std::cout << "[INFO] " << message << std::endl;
    }
    
    static void error(const std::string& message) {
        std::cout << "[ERROR] " << message << std::endl;
    }
    
    static void warn(const std::string& message) {
        std::cout << "[WARN] " << message << std::endl;
    }
};