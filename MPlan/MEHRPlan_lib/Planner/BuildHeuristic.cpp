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
    for (size_t con_idx = 0; con_idx < mdp.considerations.size(); ++con_idx) {
        SCVI(con_idx, statesByTime);
    }
}

// Single-Consideration Value Iteration Algorithm.
void Solver::SCVI(size_t con_idx, vector<size_t> &statesByTime) {
    for (auto i : statesByTime) {
        // 1. Get actions
        auto actions = mdp.getActions(*mdp.states[i]);
        if (actions->empty()) {
            continue;
        }
        // 2. Collect aggregated worth each action
        // Aggregated worth of each action
        vector<unique_ptr<WorthBase>> actionWorths;
        vector<WorthBase*> successorWorths;
        for (int a_idx = 0; a_idx < actions->size(); ++a_idx) {
            successorWorths.clear();
            vector<Successor*>* successors = MDP::getActionSuccessors(*mdp.states[i], a_idx);
            for (auto scr : *successors) {
                // Get first worth of each successor. First since this algorithm is single-objective/no multi-worth per state.
                //WorthBase* w = mData->at(scr->target)[0].expectations[con_idx];
                auto* w = mData[scr->target][0].expectations[con_idx].get();
                successorWorths.push_back(w);
            }
            actionWorths.push_back(mdp.considerations[con_idx]->gather(*successors, successorWorths, false));
        }
        //
        // 3. Find action with most preferable aggregated worth
        //
        size_t bestIdx=0;
        for (size_t a_idx = 1; a_idx < actionWorths.size(); ++a_idx) {
            if (actionWorths[a_idx]->compare(*actionWorths[bestIdx])==1) {
                bestIdx = a_idx;
            }
        }
        // 4. Set Data to most preferable worth.
        mData.at(i)[0].expectations[con_idx] = std::move(actionWorths[bestIdx]);
    }
}