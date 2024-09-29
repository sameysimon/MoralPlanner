//
// Created by Simon Kolker on 25/09/2024.
//
#include <iostream>

#include "Solver.hpp"


using namespace std;


std::vector<std::shared_ptr<Solution>> Solver::MOiLAO() {
    // Initial set of solutions has one policy.
    vector<shared_ptr<Solution>> solSet = vector<shared_ptr<Solution>>();
    vector<shared_ptr<Solution>> solSetCopy;
    shared_ptr<Solution> sol = make_shared<Solution>(mdp, true);
    solSet.push_back(sol);
    copySolSets(solSet, solSetCopy);

    int backups = 0;
    int iterations = 0;

    auto* foundStates = new unordered_set<array<int, 2>, ArrayHash>();;
    auto* expanded = new unordered_set<array<int, 2>,ArrayHash>();
    set<array<int, 2>, ArrayCompare>* Z = new set<array<int, 2>, ArrayCompare>(); // Best partial solution set.
    array<int, 2> start_state = {0,0};// Time 0, state 0

    // Do one initial backup to find out best starting action.
    backup(solSet, *mdp.states[0], 0);
    foundStates->insert(start_state);

    do {
        copySolSets(solSet, solSetCopy);
        setPostOrderDFS(Z, foundStates, solSet);
        cout << "Found States:"<<endl;
        for (auto elem : *foundStates) { cout << "t=" << elem[0] << ",s=" << elem[1] << ";  "; }
        cout << endl;
        for (const std::array<int, 2> time_state : *Z) {
            int time = time_state.at(0);
            int stateIdx = time_state.at(1);
            cout<< "Backing-up Time " << time << " State " << stateIdx << endl;
            backup(solSet, *mdp.states[stateIdx], time);
            backups++;

            expanded->insert({time, stateIdx});

        }
        cout << "There are " << solSet.size() << " undominated solutions at " << backups << " backups and " << iterations << " iterations." << endl;
        cout << SolutionSetToString(solSet);
        iterations++;
    } while (checkForUnexpandedStates(expanded, foundStates) or not hasConverged(solSet, solSetCopy));

    delete foundStates;
    delete expanded;
    delete Z;
    return solSet;
}

// Uses Depth-First-Search on states in internal SolSet to build bpsg
bool Solver::PostOrderDFSCall(int stateIdx, int time, unordered_set<std::array<int, 2>, ArrayHash>& visited, set<array<int, 2>,ArrayCompare>* bpsg, unordered_set<std::array<int, 2>, ArrayHash>* foundStates, vector<std::shared_ptr<Solution>>& solSet) {
    if (time >= mdp.horizon) {
        return false;// If post-horizon, don't search.
    }
    if (foundStates->find({time, stateIdx}) == foundStates->end()) {
        foundStates->insert({time, stateIdx});
        return false; // If already on Explicit Graph, don't search!
    }
    if (visited.find({time, stateIdx}) != visited.end()) {
        return true; // If already visited, don't search.
    }


    visited.insert({time,stateIdx});
    bool isLeaf=true;
    for(auto& sol : solSet) {
        int stateAction = sol->policy[time][stateIdx];
        auto scrs = mdp.getActionSuccessors(*mdp.states[stateIdx], stateAction);
        if (scrs==nullptr) { continue; }
        for (auto scr : *scrs) {
            PostOrderDFSCall(scr->target, time+1, visited, bpsg, foundStates, solSet);

        }
    }
    bpsg->insert({time, stateIdx});
    return true;
}

void Solver::setPostOrderDFS(set<array<int, 2>,ArrayCompare>* bpsg, unordered_set<std::array<int, 2>, ArrayHash>* foundStates, vector<std::shared_ptr<Solution>>& solSet) {
    bpsg->clear();
    unordered_set<std::array<int, 2>, ArrayHash> visited = unordered_set<std::array<int, 2>, ArrayHash>();
    PostOrderDFSCall(0, 0, visited, bpsg, foundStates, solSet);

}

bool Solver::checkForUnexpandedStates(unordered_set<array<int, 2>,ArrayHash>* expanded, unordered_set<array<int, 2>,ArrayHash>* foundStates) {
    if (expanded->size()==0 or foundStates->size()==0) {
        return true;
    }
    for (const auto& elem : *foundStates) {
        if (expanded->find(elem) == expanded->end()) {
            return true;
        }
    }
    return false;
}
