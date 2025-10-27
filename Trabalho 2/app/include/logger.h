#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>

enum class LogLevel { ERROR = 0, WARN, INFO, DEBUG };

class Logger {
    LogLevel level;

public:
    Logger() {
        std::string env = std::getenv("LOG_LEVEL") ? std::getenv("LOG_LEVEL") : "info";
        std::transform(env.begin(), env.end(), env.begin(), ::tolower);
        if (env == "error") level = LogLevel::ERROR;
        else if (env == "warn") level = LogLevel::WARN;
        else if (env == "debug") level = LogLevel::DEBUG;
        else level = LogLevel::INFO; // padrão
    }

    bool isEnabled(LogLevel msgLevel) const {
        return static_cast<int>(msgLevel) <= static_cast<int>(level);
    }

    void error(const std::string& msg) const {
        if (isEnabled(LogLevel::ERROR))
            std::cerr << "[ERROR] " << msg << std::endl;
    }

    void warn(const std::string& msg) const {
        if (isEnabled(LogLevel::WARN))
            std::cerr << "[WARN] " << msg << std::endl;
    }

    void info(const std::string& msg) const {
        if (isEnabled(LogLevel::INFO))
            std::cout << "[INFO] " << msg << std::endl;
    }

    void debug(const std::string& msg) const {
        if (isEnabled(LogLevel::DEBUG))
            std::cout << "[DEBUG] " << msg << std::endl;
    }
};

#endif
