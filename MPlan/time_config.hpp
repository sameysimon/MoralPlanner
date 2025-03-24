//
// Created by Simon Kolker on 21/03/2025.
//
#pragma once
#include <chrono>

#define TIME_METRIC_STR "microseconds"
typedef std::chrono::microseconds time_metric;
#define duration(a)


template<typename T, typename R, typename... Args>
long long timeFunc(R (T::*func)(Args...), T& obj, Args&&... args) {
    auto t1 = std::chrono::high_resolution_clock::now();
    (obj.*func)(std::forward<Args>(args)...);
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<time_metric>(t2 - t1).count();
}