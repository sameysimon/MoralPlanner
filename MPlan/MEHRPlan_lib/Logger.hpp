//
// Created by Simon Kolker on 19/03/2025.
//

#pragma once
#include <iostream>
#include <format>
#include <list>
#include "../time_config.hpp"

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
inline ushort defaultColor = 7;
static ushort LogLevel=5;

class Log {
public:
  // 0=FATAL; 1=ERROR; 2=WARN; 3=INFO; 4=DEBUG; 5=TRACE; 6=ALL
  static void setLogLevel(ushort level_) {
    LogLevel=level_;
  }
  // Write log to console with a log level; optionally set a color
  static void writeLog(const std::string &msg, ushort level_=3, ushort color=7) {
    //std::cout << "level=" << level_ << ", loglevel=" << LogLevel << " thus should print=" << (level_ <= LogLevel) << msg << std::endl;
    if (level_<= LogLevel) {
      //std::cout << "\e[0;3" << color << "m" << msg << "\e[0;3" << defaultColor << "m" <<  std::endl << std::flush;
      std::cout << msg << std::endl;
    }
  }
  template <class... Args>
  static void writeFormatLog(ushort level_=3, ushort color=7, std::format_string<Args...> fmt="{}", Args&&... args) {
    if (level_<= LogLevel) {
        writeLog(std::format(fmt, std::forward<Args>(args)...), level_, color);
    }
  }
  template <class... Args>
  static void writeFormatLog(ushort level_=3, std::format_string<Args...> fmt="{}", Args&&... args) {
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