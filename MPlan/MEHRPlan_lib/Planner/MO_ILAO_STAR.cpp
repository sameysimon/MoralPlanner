//
// Created by Simon Kolker on 25/09/2024.
//

#include <iostream>
#include <sstream>
#include "Solver.hpp"
#include "Utilitarianism.hpp"
#include "ExtractSolutions.hpp"
#include "Logger.hpp"
using namespace std;

// Setup Data to store Domain-Dependent Heuristic QValues
// Data structure is d[state_idx] = {QValue : for each undominated solution}
void Solver::build_blank_data(vector<vector<QValue>>* d) {
    for (auto & state : mdp.states) {
        auto us = vector<QValue>();
        // New QValue, fill with heuristics
        QValue qv = QValue();
        for (auto c : mdp.considerations) {
            qv.expectations.push_back(c->newHeuristic(*state));
        }
        us.push_back(qv);
        d->push_back(us);
    }
}

void clone_data(vector<vector<QValue>>& data, vector<vector<QValue>>& data_clone) {
    size_t states = data.size();
    for (int s = 0; s < states; ++s) {
        data_clone[s].clear();
        for (QValue& qv : data[s] ) {
            data_clone[s].push_back(qv);// Should *Copy* using copy constructor.
        }
    }
}

//
// MAIN ALGORITHM.
//
void Solver::MOiLAO() {
    // Initialise Multi-Worth Function and a clone. It maps state to a list of solutions' possible QValues.
    data = new vector<vector<QValue>>();
    build_blank_data(data);
    auto data_clone = new vector<vector<QValue>>();
    build_blank_data(data_clone);

    // Initialise Pi. It maps state to a vector of available actions.
    Pi = new vector<vector<int>>();
    for (int s = 0; s < mdp.states.size(); ++s) {
        Pi->emplace_back();
    }

    backups = 0;
    iterations = 0;

    // Explicitly encountered states
    foundStates = make_unique<unordered_set<int>>();
    foundStates->insert(0);

    auto* expanded = new unordered_set<int>();

    Z = new vector<int>(); // Ordered Best partial sub-graph.
    Z->emplace_back(0);


    Log::writeLog("***AO-STAR Planning***", Info);
    Log::writeFormatLog(Info, "Heuristic Planning with {} states,", data->size());

    do {
        clone_data(*data, *data_clone);
        for (const int stateIdx : *Z) {
            Log::writeLog(std::format("Backing up time {}, state: {}", mdp.states[stateIdx]->time, stateIdx), LogLevel::Info);
            backup(*mdp.states[stateIdx]);
            backups++;
            expanded->insert(stateIdx);
        }
        setPostOrderDFS(foundStates);
#ifdef DEBUG
        Log::writeLog(std::format("Reached {} Backups at iteration {}", backups, iterations), LogLevel::Info);
        Log::writeLog(std::format("Set Post Order DFS. Found {} state-times:", foundStates->size()), LogLevel::Info);
        for (auto elem : *foundStates) { Log::writeLog(std::format("   t={}, s={};", mdp.states[elem]->time, elem), LogLevel::Trace); }
        Log::writeLog(std::format("\n"), LogLevel::Info);
#endif
        iterations++;
    } while (checkForUnexpandedStates(expanded, Z) or not checkConverged(*data, *data_clone));

    // For stats later
    this->expanded_states = expanded->size();
    // Cleanup
    delete expanded;
    delete data_clone;
}

// Termination Condition
bool Solver::checkConverged(vector<vector<QValue>>& data, vector<vector<QValue>>& data_other) {
    Log::writeLog("Checking converged states...", LogLevel::Info);
    auto states = data.size();
    for (int s = 0; s < states; ++s) {
        if (data[s].size() != data_other[s].size()) {
            Log::writeLog("No convergence. Size mismatch.", LogLevel::Info);
            return false;
        }
        bool existsSimilar = false;
        for (QValue &qv : data[s] ) {
            for (QValue &qv2 : data_other[s] ) {
                if (qv.isEquivalent(qv2)) {
                    existsSimilar = true;
                    break;
                }
            }
            if (existsSimilar) { break; }
        }
        if (!existsSimilar) {
            Log::writeFormatLog(Info, "No Convergence detected on state {}.", s);
#ifdef DEBUG
            stringstream ss;
            ss << "Old:";
            for (QValue &qv : data_other[s] ) { ss << qv.toString() << "  "; }
            ss << "\nNew:";
            for (QValue &qv : data[s] ) { ss << qv.toString() << "  "; }
            Log::writeLog(ss.str(), LogLevel::Trace);
#endif
            return false;
        }
    }
    return true;
}


void Solver::backup(State& state) {
    // Values we need
    vector<QValue> candidates = vector<QValue>();// The QValue candidates (corresponding to state-actions)
    vector<int> indicesOfUndominated = vector<int>();// Indices of candidate QValues that are undominated.
    vector<int> qValueIdxToAction = vector<int>();// Maps QValue index to action index.

    // Get the values
    getCandidates(state, candidates, indicesOfUndominated, qValueIdxToAction);

    if (indicesOfUndominated.empty()) { return; }
    // Set to Data
    data->at(state.id).clear();
    for (auto elem : indicesOfUndominated) {
        data->at(state.id).push_back(candidates[elem]);
    }
    Pi->at(state.id).clear();
    // Record what actions we're using.
    for (auto elem : indicesOfUndominated) {
        Pi->at(state.id).push_back(qValueIdxToAction[elem]);
    }
}

void Solver::getCandidates(State& state, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction) {
    // Auxillary, non-requred vectors.
    vector<shared_ptr<Action>>* actions = mdp.getActions(state);
    if (actions->size()==0 or state.time>=mdp.horizon) {
        return;
    }
    for (int aIdx = 0; aIdx < actions->size(); ++aIdx) {
        vector<Successor*>* successors = MDP::getActionSuccessors(state, aIdx);
        // Get successor QValues and initialise combinations space.
        auto successorQValues = vector<vector<QValue>>();
        auto combos = vector<vector<QValue>>();
        for (auto & successor : *successors) {
            auto& q = data->at(successor->target);
            successorQValues.push_back(q);
        }
        // Generate all combinations of SuccessorQValue/theory
        // scr_1 QVals: [[A,B] / [C,D]]; scr_2 QVals: [[E,F]]
        // Combos would be scr_1: [A,B] sc_2: [E,F]; scr_1: [C,D] sc_2: [E,F].
        vector<QValue> currentcombo = vector<QValue>(successorQValues.size());
        generateCombinations(combos, successorQValues, currentcombo, 0);
        for (auto& elem : combos) {
            QValue new_qv = QValue(elem[0]);// Copy a random QValue (just for size really)
            for (int cIdx = 0; cIdx < mdp.considerations.size(); ++cIdx) {
                // Fill expectations from QValue Combination.
                std::vector<WorthBase*> comboExpects = std::vector<WorthBase*>(successors->size());
                for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
                    comboExpects[scrIdx] = elem[scrIdx].expectations[cIdx];
                }
                // Gather QValue Information.
                new_qv.expectations[cIdx] = mdp.considerations[cIdx]->gather(*successors, comboExpects, false);
            }
            // Store Candidate QValue and push back.
            candidates.push_back(new_qv);
            qValueIdxToAction.push_back(aIdx);
        }
    }
    pprune_alt(candidates, indicesOfUndominated);
#ifdef DEBUG
    stringstream ss;
    ss << "   Actions Undominated = ";
    for (auto elem : indicesOfUndominated) {
        auto a = actions->at(qValueIdxToAction[elem]);
        ss <<  (a->label) << " @ {" << candidates[elem].toString() << "}; ";
    }
    Log::writeLog(ss.str(), Trace);
#endif
}

void Solver::generateCombinations(vector<vector<QValue>>& allCombos, vector<vector<QValue>>& successorQVals, vector<QValue> current, int scr_Idx) {
    if (scr_Idx==successorQVals.size()) {
        allCombos.push_back(current);
        return;
    }
    for (auto& qv : successorQVals[scr_Idx]) {
        current[scr_Idx] = qv;
        generateCombinations(allCombos, successorQVals, current, scr_Idx+1);
    }
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
    // TODO This does more comparisons than necessary. Could be optimised.
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
            int r = mdp.CompareByConsiderations(qv, inVector[j]);
            if (r == -1) {
                isDominated=true;
                break;
            }
        }
        if (!isDominated) {
            outVector.push_back(i);
        }
    }
}

bool Solver::checkForUnexpandedStates(unordered_set<int>* expanded, vector<int>* bpsg) {
    if (expanded->size()==0 or bpsg->size()==0) {
        return true;
    }
    bool unexpanded = false;
    for (const auto& elem : *bpsg) {
        if (mdp.non_moralTheoryIdx!= -1 && mdp.states[elem]->isGoal) {
            continue;
        }
        if (expanded->find(elem) == expanded->end()) {
            return true;
        }
    }
    return unexpanded;
}

void Solver::getSolutions(vector<Policy*> &policies){
    auto se = SolutionExtracter(mdp);
    se.extractPolicies(policies, *Pi, *Z);
}