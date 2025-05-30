//
// Created by Simon Kolker on 19/03/2025.
//

#pragma once
#include <iostream>
#include <format>
#include "../time_config.hpp"
#include <utility>

enum LogLevel {
  Fatal=0,
  Error=1,
  Warn=2,
  Info=3,
  Debug=4,
  Trace=5,
  All=6
};
enum Color {
  White=7,
  Red=1,
  Green=2,
  Blue=4,
  Black=0
};

typedef std::chrono::high_resolution_clock::time_point TimeVar;
inline ushort defaultColor = White;
static ushort LogLevel=6;

class Log {
public:
  // 0=FATAL; 1=ERROR; 2=WARN; 3=INFO; 4=DEBUG; 5=TRACE; 6=ALL
  static void setLogLevel(ushort level_) {
    LogLevel=level_;
  }
  // Write log to console. Level, 0=FATAL; 1=ERROR; 2=WARN; 3=INFO; 4=DEBUG; 5=TRACE; 6=ALL
  // Optionally set a color. Color 31=red text; 32=green text 34=blue text
  static void writeLog(const std::string &msg, ushort level_=defaultColor, ushort color=defaultColor) {
    if (level_<= LogLevel) {
      std::cout << "\e[0;3" << color << "m" << msg << "\e[0;30m" << std::endl;
    }
  }
  template <class... Args>
  static void writeFormatLog(ushort level_=Debug, ushort color=defaultColor, std::format_string<Args...> fmt="{}", Args&&... args) {
    if (level_<= LogLevel) {
        writeLog(std::format(fmt, std::forward<Args>(args)...), level_, color);
    }
  }
  template <class... Args>
  static void writeFormatLog(ushort level_=Debug, std::format_string<Args...> fmt="{}", Args&&... args) {
      if (level_<= LogLevel) {
        writeLog(std::format(fmt, std::forward<Args>(args)...), level_);
      }
  }
  template <class... Args>
  static void writeFormatLog(std::format_string<Args...> fmt="{}", Args&&... args) {
    // Use default debug level
    if (Debug <= LogLevel) {
      writeLog(std::format(fmt, std::forward<Args>(args)...));
    }
  }

  static std::string listToString(std::list<int>& numbers) {
    std::string result = "[";
    bool first = true;
    for (int n : numbers) {
      if (!first) result += ", ";
      result += std::format("{}", n);
      first = false;
    }
    result += "]";
    return result;
  }



};

