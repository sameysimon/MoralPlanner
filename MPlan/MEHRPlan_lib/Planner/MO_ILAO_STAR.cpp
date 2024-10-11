//
// Created by Simon Kolker on 25/09/2024.
//

#include <iostream>
#include <sys/stat.h>

#include "Solver.hpp"
using namespace std;

void Solver::build_blank_data(vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>* d) {
    for (int t=0; t<mdp.horizon+1; t++) {
        auto stateVec = vector<unordered_set<QValue,QValueHash,QValueEqual>>();
        for (int s = 0; s < mdp.states.size(); s++) {
            auto us = unordered_set<QValue,QValueHash,QValueEqual>();
            QValue qv = QValue();
            for (int theoryIdx=0; theoryIdx < mdp.theories.size(); theoryIdx++) {
                qv.expectations.push_back(mdp.theories[theoryIdx]->newHeuristic(*mdp.states[s]));
            }
            us.insert(qv);
            stateVec.push_back(us);
        }
        d->push_back(stateVec);
    }
}

void clone_data(vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>& data, vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>& data_clone) {
    int horizon = data.size();
    int states = data[0].size();
    for (int t = 0; t < horizon; ++t) {
        for (int s = 0; s < states; ++s) {
            data_clone.at(t)[s].clear();
            for (auto qv : data.at(t)[s] ) {
                QValue qv_clone = QValue(qv);
                data_clone.at(t)[s].insert(qv_clone);
            }
        }
    }
}
bool checkConverged(vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>& data, vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>& data_other) {
    int horizon = data.size();
    int states = data[0].size();
    for (int t = 0; t < horizon; ++t) {
        for (int s = 0; s < states; ++s) {
            if (data[t][s].size() != data_other[t][s].size()) {
                return false;
            }
            bool existsSimilar = false;
            for (QValue qv : data[t][s] ) {
                for (QValue qv2 : data_other[t][s] ) {
                    if (qv.isEquivalent(qv2)) {
                        existsSimilar = true;
                        break;
                    }
                }
                if (existsSimilar) { break; }
            }
            if (!existsSimilar) {
                return false;
            }
        }
    }
    return true;
}


void Solver::MOiLAO() {
    // Initialise Multi-Worth Function and a clone.
    data = new vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>();
    build_blank_data(data);
    auto data_clone = new vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>();
    build_blank_data(data_clone);

    // Pi maps time/state to available actions.
    Pi = new vector<vector<unordered_set<int>>>();
    for (int t = 0; t < mdp.horizon; ++t) {
        Pi->emplace_back(vector<unordered_set<int>>());
        for (int s = 0; s < mdp.states.size(); ++s) {
            Pi->at(t).emplace_back(unordered_set<int>());// TODO Maybe don't need this, try with/without?
        }
    }

    backups = 0;
    iterations = 0;

    foundStates = new unordered_set<array<int, 2>, ArrayHash>();// Explicitly encountered states
    auto* expanded = new unordered_set<array<int, 2>,ArrayHash>();
    Z = new set<array<int, 2>, ArrayCompare>(); // Best partial solution set.

    // Initialise
    array<int, 2> start_state = {0,0};// Time 0, state 0
    foundStates->insert(start_state);
    Z->insert(start_state);

    do {
        clone_data(*data, *data_clone);

        for (const std::array<int, 2> time_state : *Z) {
            int time = time_state.at(0);
            int stateIdx = time_state.at(1);
#ifdef DEBUG
            cout<< "Backing-up Time " << time << " State " << stateIdx << endl;
#endif
            backup(*mdp.states[stateIdx], time);
            backups++;
            expanded->insert({time, stateIdx});
        }

        setPostOrderDFS(foundStates);
#ifdef DEBUG
        cout << "Finished Backups at iteration #" << iterations << endl;
        cout << "Set Post Order DFS: " << endl;
        cout << "Found " << foundStates->size() << " states-times:"<<endl;
        for (auto elem : *foundStates) { cout << "t=" << elem[0] << ",s=" << elem[1] << ";  "; }
        cout << endl << endl;;
#endif
        cout << "We are at " << backups << " backups and " << iterations << " iterations." << endl;
        iterations++;
    } while (checkForUnexpandedStates(expanded, Z) or not checkConverged(*data, *data_clone));


    this->expanded_states = expanded->size();
    // Cleanup
    delete expanded;
    delete data_clone;
    for (auto elem : Pi->at(0)[0]) { std::cout << elem << endl; }

}

void generateCombinations(vector<vector<QValue>>& allCombos, vector<unordered_set<QValue,QValueHash,QValueEqual>>& successorQVals, vector<QValue> current, int scr_Idx) {
    if (scr_Idx==successorQVals.size()) {
        allCombos.push_back(current);
        return;
    }
    for (auto& qv : successorQVals[scr_Idx]) {
        current[scr_Idx] = qv;
        generateCombinations(allCombos, successorQVals, current, scr_Idx+1);
    }
}

void Solver::backup(State& state, int time) {
    // Values we need
    vector<int> indicesOfUndominated = vector<int>();
    vector<int> qValueIdxToAction = vector<int>();
    vector<QValue> candidates = vector<QValue>();

    // Get the values
    getCandidates(state, time, candidates, indicesOfUndominated, qValueIdxToAction);

    if (indicesOfUndominated.empty()) { return; }
    // Set to Data
    data->at(time)[state.id].clear();
    for (auto elem : indicesOfUndominated) {
        data->at(time)[state.id].insert(candidates[elem]);
    }
    Pi->at(time)[state.id].clear();
    // Record what actions we're using.
    for (auto elem : indicesOfUndominated) {
        Pi->at(time)[state.id].insert(qValueIdxToAction[elem]);
    }
}

void Solver::getCandidates(State& state, int time, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction) {
    // Auxillary, non-requred vectors.
    vector<Action*>* actions = mdp.getActions(state, time);
    if (actions->size()==0 or time>=mdp.horizon) {
        return;
    }
    for (int aIdx = 0; aIdx < actions->size(); ++aIdx) {
        vector<Successor*>* successors = mdp.getActionSuccessors(state, aIdx);
        // Get successor QValues and initialise combinations space.
        auto successorQValues = vector<unordered_set<QValue,QValueHash,QValueEqual>>();
        auto combos = vector<vector<QValue>>();
        for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
            auto& q = data->at(time+1)[successors->at(scrIdx)->target];
            successorQValues.push_back(q);
        }
        // Generate all combinations of SuccessorQValue/theory
        // scr_1 QVals: [[A,B] / [C,D]]; scr_2 QVals: [[E,F]]
        // Combos would be scr_1: [A,B] sc_2: [E,F]; scr_1: [C,D] sc_2: [E,F].
        vector<QValue> currentcombo = vector<QValue>(successorQValues.size());
        generateCombinations(combos, successorQValues, currentcombo, 0);
        for (auto& elem : combos) {
            QValue new_qv = QValue(elem[0]);// Copy a random QValue (just for size really)
            for (int theoryIdx = 0; theoryIdx < mdp.theories.size(); ++theoryIdx) {
                // Fill expectations from QValue Combination.
                std::vector<WorthBase*> comboExpects = std::vector<WorthBase*>(successors->size());
                for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
                    comboExpects[scrIdx] = elem[scrIdx].expectations[theoryIdx];
                }
                // Gather QValue Information.
                new_qv.expectations[theoryIdx] = mdp.theories[theoryIdx]->gather(*successors, comboExpects);
            }
            // Store Candidate QValue and push back.
            candidates.push_back(new_qv);
            qValueIdxToAction.push_back(aIdx);
        }
    }
    pprune_alt(candidates, indicesOfUndominated);
#ifdef DEBUG
    std::cout << "Actions Undominated = ";
    for (auto elem : indicesOfUndominated) {
        auto a = actions->at(qValueIdxToAction[elem]);
        cout << qValueIdxToAction[elem] << " (" << *(a->label) <<  "); ";
    }
    cout << endl;
#endif
}





void Solver::pprune_alt(std::vector<QValue>& inVector, std::vector<int>& outVector) {
    if (inVector.size()==0) {return; };

    if (inVector.size()==0) {return;}
    for (int i=0; i<inVector.size(); i++) {
        auto& qv = inVector[i];
        bool isDominated=false;
        for (auto& qv_other : inVector) {
            int r = mdp.compareQValues(qv, qv_other, false);
            if (r==-1) {
                isDominated=true;
                break;
            }
        }
        if (!isDominated) {
            outVector.push_back(i);
        }


    }

}




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

bool Solver::checkForUnexpandedStates(unordered_set<array<int, 2>,ArrayHash>* expanded, set<array<int, 2>, ArrayCompare>* bpsg) {
    if (expanded->size()==0 or bpsg->size()==0) {
        return true;
    }
    bool unexpanded = false;
    for (const auto& elem : *bpsg) {
        if (mdp.states[elem[1]]->isGoal) {
            continue;
        }
        if (expanded->find(elem) == expanded->end()) {
            return true;
        }
    }
    return unexpanded;
}





vector<shared_ptr<Solution>>* Solver::extractSolutions() {
    auto solSetSet = unordered_set<shared_ptr<Solution>, SolutionHash, SolutionCompare>();

    solutionRecurse(0, 0, make_shared<Solution>(mdp, true), solSetSet);
    auto solSet = new vector<shared_ptr<Solution>>();
    solSet->insert(solSet->end(), solSetSet.begin(), solSetSet.end());
    return solSet;
}

// Returns True if a dead end, False if there are branches.
bool Solver::solutionRecurse(int time, int stateIdx, shared_ptr<Solution> currSol, unordered_set<shared_ptr<Solution>, SolutionHash, SolutionCompare>& solSet) {
    bool iAmDeadEnd = true;
    if (time >= mdp.horizon) {
        return true;
    }

    vector<int> indicesOfUndominated = vector<int>();
    vector<int> qValueIdxToAction = vector<int>();
    vector<QValue> candidates = vector<QValue>();
    // Do a backup to get actions and values.
    getCandidates(*mdp.states[stateIdx], time, candidates, indicesOfUndominated, qValueIdxToAction);
    sort(indicesOfUndominated.begin(), indicesOfUndominated.end());

    if (indicesOfUndominated.empty()) { return true;}
    if (indicesOfUndominated.size()==1) { iAmDeadEnd=true;}
    if (indicesOfUndominated.size()>1) { iAmDeadEnd=false;}


    auto newSolutions = vector<shared_ptr<Solution>>();
    for (int idx=0; idx<indicesOfUndominated.size(); idx++) {
        if (idx==0) {
            currSol->setAction(*mdp.states[stateIdx], time, qValueIdxToAction[idx]);
            currSol->setToQValue(*mdp.states[stateIdx], time, candidates[idx]);
            newSolutions.push_back(currSol);
        } else {
            auto newSol = make_shared<Solution>(*currSol);
            newSol->setAction(*mdp.states[stateIdx], time, qValueIdxToAction[idx]);
            newSol->setToQValue(*mdp.states[stateIdx], time, candidates[idx]);
            newSolutions.push_back(newSol);
        }
    }

    vector<bool> isSolDeadEnd(mdp.states.size(), false);

    for (int sIdx = 0; sIdx < newSolutions.size(); sIdx++) {
        auto sol = newSolutions[sIdx];
        auto successors = mdp.getActionSuccessors(*mdp.states[stateIdx], sol->policy[time][stateIdx]);
        for (auto scr : *successors) {
            if (solutionRecurse(time+1, scr->target, sol, solSet)) {
                isSolDeadEnd[sIdx] = true;// This successor does not branch and is a dead end.
            }
        }
    }
    if (iAmDeadEnd and !(time==0 and stateIdx==0)) {
        return true;// None of my successors branch, this state is a dead end.
    }
    // At least one of my successors branch, thus I am not a dead end.
    for (int sIdx = 0; sIdx < newSolutions.size(); sIdx++) {
        if (isSolDeadEnd[sIdx]) {
            solSet.insert(newSolutions[sIdx]);// Add dead ends to solSet.
        }
    }
    return true;// I am not a dead end.
}
