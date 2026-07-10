//
// Created by Simon Kolker on 03/07/2026.
//

#pragma once
#include <random>

class MPlanRandom {
    inline static thread_local std::mt19937 rng{std::random_device{}()};
public:
    inline static unsigned mSeed = 0;
    inline static bool mIsInitialised = false;

    static void SetSeed(unsigned seed) {
        mSeed = seed;
        rng.seed(seed);
        mIsInitialised = true;
    }

    static std::mt19937& GetGenerator() {
        if (!mIsInitialised) {
            rng.seed(std::random_device{}());
            mIsInitialised = true;
        }
        return rng;
    }
};