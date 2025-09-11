//
// Created by Simon Kolker on 06/06/2025.
//
#include "Solver.hpp"


void Solver::BuildIndependentHeuristic() {
    vector<size_t> statesByTime(mdp.states.size());
    std::iota(statesByTime.begin(), statesByTime.end(), 0);
    std::sort(statesByTime.begin(), statesByTime.end(), [&](size_t a, size_t b) {
        return mdp.states[a]->time > mdp.states[b]->time;
    });
#if DEBUG
    // asserts descending time order lol
    int prev = 999999;
    for (auto i : statesByTime) {
        assert(mdp.states[i]->time >= prev);
        prev = mdp.states[i]->time;
    }
#endif
    //
    for (size_t con_idx = 0; con_idx < mdp.considerations.size(); ++con_idx) {
        SOVI(con_idx, statesByTime);
    }
}

void Solver::SOVI(size_t con_idx, vector<size_t> &statesByTime) {
    bool hasConverged = false;
    while (hasConverged == false) {
        for (auto i : statesByTime) {
            auto actions = mdp.getActions(*mdp.states[i]);// Every action
            // 1. Collect aggregated worth of each action
            auto actionWorths = vector<WorthBase*>();// Aggregated worth of each action
            for (int a_idx = 0; a_idx < actions->size(); ++a_idx) {
                vector<Successor*>* successors = MDP::getActionSuccessors(*mdp.states[i], a_idx);
                vector<WorthBase*> successorWorths;//Worth of successor states
                for (auto scr : *successors) {
                    WorthBase* w = (*mData)[scr->target][0].expectations[con_idx];
                    successorWorths.push_back(w);
                }
                // Add action's aggregated worth
                actionWorths.push_back(mdp.considerations[con_idx]->gather(*successors, successorWorths, false));
            }
            // 2. Find most action with most preferable worth
            size_t bestIdx=0;
            for (size_t a_idx = 1; a_idx < actionWorths.size(); ++a_idx) {
                if (actionWorths[a_idx]->compare(*actionWorths[bestIdx])==1) {
                    bestIdx = a_idx;
                }
            }
            // 3. Set Data to most preferable worth.
            (*mData)[i][0].expectations[con_idx] = actionWorths[bestIdx];
        }
        hasConverged = hasConsiderationConverged(con_idx, *mData, *mDataClone);
    }

}

bool Solver::hasConsiderationConverged(size_t con_idx, vector<vector<QValue>>& d, vector<vector<QValue>>& d_) const {
    for (int i = 0; i < mdp.states.size(); ++i) {
        if (! d[i][0].expectations[con_idx]->isEquivalent(*d_[i][0].expectations[con_idx])) {
            return false;
        }
    }
    return true;
}