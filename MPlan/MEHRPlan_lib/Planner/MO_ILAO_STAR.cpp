//
// Created by Simon Kolker on 25/09/2024.
//

#include <iostream>
#include "Solver.hpp"
#include "Utilitarianism.hpp"
#include "ExtractSolutions.hpp"
using namespace std;

// Setup Data to store QValues
void Solver::build_blank_data(vector<vector<vector<QValue>>>* d) {
    for (int t=0; t<mdp.horizon+1; t++) {
        auto stateVec = vector<vector<QValue>>();
        for (int s = 0; s < mdp.states.size(); s++) {
            auto us = vector<QValue>();
            QValue qv = QValue();
            for (int theoryIdx=0; theoryIdx < mdp.theories.size(); theoryIdx++) {
                qv.expectations.push_back(mdp.theories[theoryIdx]->newHeuristic(*mdp.states[s]));
            }
            us.push_back(qv);
            stateVec.push_back(us);
        }
        d->push_back(stateVec);
    }
}

void clone_data(vector<vector<vector<QValue>>>& data, vector<vector<vector<QValue>>>& data_clone) {
    int horizon = data.size();
    int states = data[0].size();
    for (int t = 0; t < horizon; ++t) {
        for (int s = 0; s < states; ++s) {
            data_clone.at(t)[s].clear();
            for (auto qv : data.at(t)[s] ) {
                QValue qv_clone = QValue(qv);
                data_clone.at(t)[s].push_back(qv_clone);
            }
        }
    }
}

//
// MAIN ALGORITHM.
//

void Solver::MOiLAO() {
    // Initialise Multi-Worth Function and a clone.
    data = new vector<vector<vector<QValue>>>();
    build_blank_data(data);
    auto data_clone = new vector<vector<vector<QValue>>>();
    build_blank_data(data_clone);

    // Initialise Pi. It maps time/state to available actions.
    Pi = new vector<vector<vector<int>>>();
    for (int t = 0; t < mdp.horizon; ++t) {
        Pi->emplace_back(vector<vector<int>>());
        for (int s = 0; s < mdp.states.size(); ++s) {
            Pi->at(t).emplace_back(vector<int>());// TODO Maybe don't need this, try with/without?
        }
    }

    backups = 0;
    iterations = 0;

    foundStates = new unordered_set<array<int, 2>, ArrayHash>();// Explicitly encountered states
    auto* expanded = new unordered_set<array<int, 2>,ArrayHash>();
    Z = new set<array<int, 2>, ArrayCompare>(); // Best partial grpah.

    // Initialise foundStates and Best partial sub graph.
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
            if ((time==0 && stateIdx==0) && iterations==20) {
                time = time;
            }
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
        cout << "We are at " << backups << " backups and " << iterations << " iterations." << endl;
#endif

        iterations++;
    } while (checkForUnexpandedStates(expanded, Z) or not checkConverged(*data, *data_clone));


    this->expanded_states = expanded->size();

    // Cleanup
    delete expanded;
    delete data_clone;
}

// Termination Condition
bool Solver::checkConverged(vector<vector<vector<QValue>>>& data, vector<vector<vector<QValue>>>& data_other) {
    auto horizon = data.size();
    auto states = data[0].size();
    for (int t = 0; t < horizon; ++t) {
        for (int s = 0; s < states; ++s) {
            if (data[t][s].size() != data_other[t][s].size()) {
                return false;
            }
            bool existsSimilar = false;
            for (auto qv : data[t][s] ) {
                for (auto qv2 : data_other[t][s] ) {
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


void generateCombinations(vector<vector<QValue>>& allCombos, vector<vector<QValue>>& successorQVals, vector<QValue> current, int scr_Idx) {
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
        data->at(time)[state.id].push_back(candidates[elem]);
    }
    Pi->at(time)[state.id].clear();
    // Record what actions we're using.
    for (auto elem : indicesOfUndominated) {
        Pi->at(time)[state.id].push_back(qValueIdxToAction[elem]);
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
        auto successorQValues = vector<vector<QValue>>();
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
        cout <<  *(a->label) << " @ {" << candidates[elem].toString() << "}; ";
    }
    cout << endl;
#endif
    int x = 5;
}





void Solver::pprune_alt(std::vector<QValue>& inVector, std::vector<int>& outVector) {
    if (inVector.size()==0) {return; };

    vector<bool> inBudget;
    bool anyInBudget = false;
    if (mdp.non_moralTheoryIdx != -1) {
        for (int i=0; i<inVector.size(); i++) {
            inBudget.push_back(mdp.isQValueInBudget(inVector[i]));
            if (inBudget[i]) {
                anyInBudget = true;
            }
        }
    }

    for (int i=0; i<inVector.size(); i++) {
        if (anyInBudget && !inBudget[i]) {
            continue; // Over budget QValues cannot be undominated/added to outVector
        }
        auto& qv = inVector[i];
        bool isDominated=false;
        for (int j = 0; j < inVector.size(); j++) {
            if (j==i) continue;
            if (anyInBudget && !inBudget[j]) {
                continue;// Over budget QValues cannot dominate anything.
            }
            int r = mdp.compareQValues(qv, inVector[j], false);
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


bool Solver::checkForUnexpandedStates(unordered_set<array<int, 2>,ArrayHash>* expanded, set<array<int, 2>, ArrayCompare>* bpsg) {
    if (expanded->size()==0 or bpsg->size()==0) {
        return true;
    }
    bool unexpanded = false;
    for (const auto& elem : *bpsg) {
        if (mdp.non_moralTheoryIdx!= -1 && mdp.states[elem[1]]->isGoal) {
            continue;
        }
        if (expanded->find(elem) == expanded->end()) {
            return true;
        }
    }
    return unexpanded;
}

vector<Policy*>* Solver::getSolutions(){
    auto se = SolutionExtracter(mdp);
    return se.extractPolicies(*Pi, *Z);
}