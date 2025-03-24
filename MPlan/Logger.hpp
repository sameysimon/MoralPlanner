//
// Created by Simon Kolker on 19/03/2025.
//

#pragma once
#include <iostream>
#include <format>
#include "time_config.hpp"
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

typedef std::chrono::high_resolution_clock::time_point TimeVar;
static ushort LogLevel=6;
class Log {
public:
  // 0=FATAL; 1=ERROR; 2=WARN; 3=INFO; 4=DEBUG; 5=TRACE; 6=ALL
  static void setLogLevel(ushort level_) {
    LogLevel=level_;
  }
  // Write log to console. Level, 0=FATAL; 1=ERROR; 2=WARN; 3=INFO; 4=DEBUG; 5=TRACE; 6=ALL
  // Optionally set a color. Color 31=red text; 32=green text 34=blue text
  static void writeLog(const std::string &msg, ushort level_=Debug, ushort color=Fatal) {
    if (level_<= LogLevel) {
      std::cout << "\33[" << color << "m" << msg << "\33[0m" << std::endl;
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

