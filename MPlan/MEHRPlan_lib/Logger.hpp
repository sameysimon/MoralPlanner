//
// Created by Simon Kolker on 19/03/2025.
//

#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <list>
#include <string>
#include <utility>

enum class LogLevel : std::uint8_t {
    Fatal = 0,
    Error = 1,
    Warn  = 2,
    Info  = 3,
    Debug = 4,
    Trace = 5,
    All   = 6
};

enum class Color : std::uint8_t {
    Black = 0,
    Red   = 1,
    Green = 2,
    Blue  = 4,
    White = 7
};

typedef std::chrono::high_resolution_clock::time_point TimeVar;

class Log {
public:
    static void setLogLevel(LogLevel level) {
        currentLevel = level;
    }

    static LogLevel getLogLevel() {
        return currentLevel;
    }

    static void writeLog(const std::string& message, LogLevel level = LogLevel::Info) {
        if (shouldLog(level)) {
            std::cout << message << '\n';
        }
    }

    template<class... Args>
    static void writeFormatLog(LogLevel level, std::format_string<Args...> format, Args&&... args) {
        if (shouldLog(level)) {
            std::cout << std::format(format, std::forward<Args>(args)...) << '\n';
        }
    }

    template<class... Args>
    static void writeFormatLog(std::format_string<Args...> format, Args&&... args) {
        writeFormatLog(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

private:
    inline static LogLevel currentLevel = LogLevel::Trace;

    static bool shouldLog(LogLevel messageLevel) {
        return static_cast<std::uint8_t>(messageLevel) <= static_cast<std::uint8_t>(currentLevel);
    }
};