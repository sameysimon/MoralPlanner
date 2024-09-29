//
//  MoralTheory.cpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#include "MoralTheory.hpp"
#include "Expecter.hpp"


Expecter* MoralTheory::makeExpecter(int size, int horizon) {
    Expecter* ex = new Expecter(size, horizon, this);
    for (int t = 0; t < horizon+1; t++) {
        for (int i = 0; i < size; i++) {
            ex->expectations[t][i] = newWorth();
        }
    }

    return ex;
}
Expecter* MoralTheory::makeHeuristicExpecter(std::vector<State*>& states, int horizon) {
    Expecter* ex = new Expecter(states.size(), horizon, this);
    for (int t = 0; t < horizon+1; t++) {
        for (int i = 0; i < states.size(); i++) {
            ex->expectations[t][i] = newHeuristic(*states[i]);
        }
    }

    return ex;
}

