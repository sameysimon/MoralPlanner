//
// Created by psiko on 9/13/26.
//

#pragma once
#include <cstddef>
using namespace std;
class Stats {
    public:
    // Planning
    static inline size_t paretoComparisons = 0;
    static inline size_t iAOStarParetoFilters = 0;
    static inline size_t iAOStarLoops = 0;

    static inline size_t backups = 0;

    static inline size_t expandedStates = 0;
    static inline size_t policiesGenerated = 0;

    static void Reset() {

    }

};
