//
// Created by Simon Kolker on 21/03/2025.
//
#pragma once
#include <chrono>
#include <ctime>

#define TIME_METRIC_STR "microseconds"
typedef std::chrono::microseconds time_metric;
#define duration(a)


template<typename T, typename R, typename... Args>
long long WallTime(R (T::*func)(Args...), T& obj, Args&&... args) {
    auto t1 = std::chrono::high_resolution_clock::now();
    (obj.*func)(std::forward<Args>(args)...);
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<time_metric>(t2 - t1).count();
}

static inline long long now_process_cpu_ns() {
    timespec ts{};
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return (long long)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

template <class F, class... Args>
long long CPUTime(F&& f, Args&&... args) {
    auto t1 = now_process_cpu_ns();
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    auto t2 = now_process_cpu_ns();
    return (t2 - t1) / 1000; // microseconds
}
