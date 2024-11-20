//
// Created by Simon Kolker on 23/10/2024.
//
#include "Solver.hpp"
using namespace std;

// Uses Depth-First-Search on states in internal SolSet to build bpsg (Z)
bool Solver::PostOrderDFSCall(int stateIdx, int time, unordered_set<std::array<int, 2>, ArrayHash>& visited, unordered_set<std::array<int, 2>, ArrayHash>* foundStates) {
    if (visited.find({time, stateIdx}) != visited.end()) {
        return false; // If already visited, don't search.
    }
    visited.insert({time,stateIdx});
    if (time >= mdp.horizon) {
        return false;
    }
    if (foundStates->find({time, stateIdx}) == foundStates->end()) {
        foundStates->insert({time, stateIdx});
        return true; // If not already on Explicit Graph, don't search!
    }


    for(auto stateAction : Pi->at(time)[stateIdx]) {
        auto scrs = mdp.getActionSuccessors(*mdp.states[stateIdx], stateAction);
        if (scrs==nullptr) { continue; }

        for (auto scr : *scrs) {
            int ttime = time+1;
            if (PostOrderDFSCall(scr->target, ttime, visited, foundStates)) {
                Z->insert({ttime, scr->target});
            }
        }
        Z->insert({time, stateIdx});
    }
    return true;
}


void Solver::setPostOrderDFS(unordered_set<std::array<int, 2>, ArrayHash>* foundStates) {
    Z->clear();
    unordered_set<std::array<int, 2>, ArrayHash> visited = unordered_set<std::array<int, 2>, ArrayHash>();
    PostOrderDFSCall(0, 0, visited, foundStates);

}