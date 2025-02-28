//
// Created by Simon Kolker on 23/10/2024.
//
#include "Solver.hpp"
using namespace std;

// Uses Depth-First-Search on states in internal SolSet to build bpsg (Z)
bool Solver::PostOrderDFSCall(int stateIdx, int time, unordered_set<int>& visited, unique_ptr<unordered_set<int>>& foundStates) {
    if (visited.find(stateIdx) != visited.end()) {
        return false; // If already visited, don't search.
    }
    visited.insert(stateIdx);
    if (time >= mdp.horizon) {
        return false;
    }
    if (foundStates->find(stateIdx) == foundStates->end()) {
        foundStates->insert(stateIdx);
        return true; // If not already on Explicit Graph, don't search!
    }


    for(auto stateAction : Pi->at(stateIdx)) {
        auto scrs = mdp.getActionSuccessors(*mdp.states[stateIdx], stateAction);
        if (scrs==nullptr) { continue; }

        for (auto scr : *scrs) {
            int ttime = time+1;
            if (PostOrderDFSCall(scr->target, ttime, visited, foundStates)) {
                Z->emplace_back(scr->target);
            }
        }
    }
    //Z->emplace_back(stateIdx);
    return true;
}


void Solver::setPostOrderDFS(unique_ptr<unordered_set<int>>& foundStates) {
    Z->clear();
    unordered_set<int> visited = unordered_set<int>();

    PostOrderDFSCall(0, 0, visited, foundStates);
    Z->emplace_back(0);
}