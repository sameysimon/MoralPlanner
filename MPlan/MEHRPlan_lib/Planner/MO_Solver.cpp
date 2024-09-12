//
// Created by Simon Kolker on 29/08/2024.
//
#include <iostream>
#include <sstream>
#include "Solver.hpp"

using namespace std;

std::string Solver::SolutionSetToString(std::vector<std::shared_ptr<Solution>>& solSet) {
    stringstream stream;
    int solNum = 0;
    for (auto sol : solSet) {
        stream << "Solution #" << solNum << endl;
        stream << sol->worthToString() << endl;
        stream << sol->policyToString() << endl;
        solNum++;
    }
    stream << endl;
    return stream.str();
}

bool hasConverged(vector<shared_ptr<Solution>>& solSet, vector<shared_ptr<Solution>>& solSet_) {
    // TODO seems broken.
    if (solSet.size() != solSet_.size()) {
        return false;
    }
    // Below is maybe too slow + strict.
    // Also assumes similar solutions will be placed in same index. Should be true.
    // Should be around
    for (int i = 0; i < solSet.size(); ++i) {
        for (int expIdx = 0; expIdx < solSet[i]->expecters.size(); ++expIdx) {
            Expecter* e = solSet[i]->expecters[expIdx];
            Expecter* e_ = solSet_[i]->expecters[expIdx];

            if (not e->isEquivalent(*e_))
                return false;
        }
    }
    return true;
    // TODO add some distance measure between solutions here!?!?
    // solSet[i].expecters[m].expectations[s][t]
    // x solutions; M moral theories; S times T worth values.
    // 'distance' between solutions is average distance between solutions for theories?
    // 'distance between solutions
}

vector<shared_ptr<Solution>> Solver::MOValueIteration() {
    // Initial set of solutions has one policy.
    vector<shared_ptr<Solution>> solSet = vector<shared_ptr<Solution>>();
    vector<shared_ptr<Solution>> solSetCopy = solSet;
    shared_ptr<Solution> sol = make_shared<Solution>(mdp);
    solSet.push_back(sol);
    int backups = 0;
    int iterations = 0;

    do {
        // Copy Sol_Set
        solSetCopy = solSet;
        for (int i=0; i<solSet.size(); ++i)
            solSetCopy[i] = make_shared<Solution>(*solSet[i]);

        // Bellman Backup on all states + actions
        for (int t = 0; t < mdp.horizon; t++) {
            for (int i = 0; i < mdp.total_states; i++) {
                backup(solSet, *mdp.states[i], t);
                backups++;
            }
        }
        std::cout << "There are " << solSet.size() << " undominated solutions at " << backups << " backups and " << iterations << " iterations." << endl;
        std::cout << SolutionSetToString(solSet);
        if (backups % 1000 == 0) { std::cout << "Iteration #" << backups << endl; }
        iterations++;
    }
    while (not hasConverged(solSet, solSetCopy));
    std::cout << "Done!";
    return solSet;
}

void Solver::backup(vector<shared_ptr<Solution>>& solSet, State& state, int time) {
    //
    // 1. Get solution candidates's relevant Q-Values.
    //
    vector<Action*>* actions = mdp.getActions(state, time);
    if (actions->size()==0)
        return;
    vector<QValue> qValues = vector<QValue>();
    vector<int> qValueIdxToAction = vector<int>();
    vector<int> qValueIdxToSol = vector<int>();

    qValues.reserve(solSet.size() * actions->size());
    qValueIdxToAction.reserve(solSet.size() * actions->size());
    qValueIdxToSol.reserve(solSet.size() * actions->size());

    vector<int> actionsAtCurrent = vector<int>();

    for (int sIdx=0; sIdx < solSet.size(); sIdx++) {
        actionsAtCurrent.push_back(solSet[sIdx]->policy[time][state.id]);
        for (int aIdx = 0; aIdx < actions->size(); aIdx++) {
            vector<Successor*>* successors = mdp.getActionSuccessors(state, aIdx);
            qValues.push_back(solSet[sIdx]->vectorGather(*successors, time+1));
            qValueIdxToAction.push_back(aIdx);
            qValueIdxToSol.push_back(sIdx);
        }
    }
    //
    // 2. Prune Q Values
    //
    vector<shared_ptr<Solution>> newSolSet = vector<shared_ptr<Solution>>();
    vector<int> indicesOfUndominated = vector<int>();
    int prevSolIdx=-1;
    pprune(qValues, indicesOfUndominated);
    std::sort(indicesOfUndominated.begin(), indicesOfUndominated.end());// Should remain sorted.

    //
    // 3. Update Solution set to match pruned Q-Values.
    //
    for (auto idx : indicesOfUndominated) {
        shared_ptr<Solution> solToUpdate;
        if (prevSolIdx != qValueIdxToSol[idx]) {
            solToUpdate = solSet[qValueIdxToSol[idx]];
        } else {
            solToUpdate = make_shared<Solution>(*solSet[qValueIdxToSol[idx]]);
        }

        solToUpdate->setToQValue(state, time, qValues[idx]);
        solToUpdate->setAction(state, time, *actions->at(qValueIdxToAction[idx]));
        newSolSet.push_back(solToUpdate);
        prevSolIdx = qValueIdxToSol[idx];
    }

    //
    //4. Prune away duplicate policies.
    //
    solSet.clear();
    vector<size_t> policyHashes = vector<size_t>();
    for (auto sol : newSolSet) {
        size_t h = sol->policyHash(-1, -1, -1);
        if (std::find(policyHashes.begin(), policyHashes.end(), h) != policyHashes.end()) {
            continue; // found
        }
        policyHashes.push_back(h);
        solSet.push_back(sol);
    }
}

void Solver::pprune(std::vector<QValue>& inSet, std::vector<int>& outSet) {
    // TODO MUST BE TESTED!
    if (inSet.size()==0) {return;}
    // Will hold index of QValues, starting at index 1.
    std::list<int> inSetIndex = list<int>(inSet.size());
    std::iota(inSetIndex.begin(), inSetIndex.end(), 0);

    while (inSetIndex.size() > 0) {
        QValue& dominantQ = inSet[inSetIndex.front()];
        int dominantIdx = inSetIndex.front();
        inSetIndex.erase(inSetIndex.begin());

        auto it = inSetIndex.begin();
        while (it != inSetIndex.end()) {
            QValue& inSetQValue = inSet[*it];
            int result = mdp.compareQValues(dominantQ, inSetQValue);


            if (result == -1) {//(*inSet[i] > *dominant) New dominating Solution.
                // Set new dominant
                dominantQ = inSet[*it];
                dominantIdx = *it;
                // Remove new dominant
                it = inSetIndex.erase(it);
            } else if (result == 1) { //(*dominant > *inSet[i]) Dominant solution dominates inSet solution.
                it = inSetIndex.erase(it);
            } else {
                ++it;
            }
        }
        outSet.push_back(dominantIdx);
    }
}












