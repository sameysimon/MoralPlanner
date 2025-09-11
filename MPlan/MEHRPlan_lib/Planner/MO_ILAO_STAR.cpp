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
void Solver::build_blank_data(vector<vector<QValue>> *d) {
    d->reserve(mdp.states.size());
    for (auto & state : mdp.states) {
        auto us = vector<QValue>(1);
        // New QValue, fill with heuristics
        QValue qv = QValue();
        qv.expectations.reserve(mdp.considerations.size());
        for (auto c : mdp.considerations) {
            qv.expectations.push_back(c->newHeuristic(*state));
        }
        us[0] = qv;
        d->push_back(us);
    }
}

void Solver::clone_data(vector<vector<QValue>>& d, vector<vector<QValue>>& d_) {
    size_t states = d.size();
    for (int s = 0; s < states; ++s) {
        d_[s].clear();
        for (QValue& qv : d[s] ) {
            d_[s].push_back(qv);// Should *Copy* using copy constructor.
        }
    }
}

//
// MAIN ALGORITHM.
//

void Solver::MOiAO_Star() {
    Log::writeLog("***AO-STAR Planning***", Debug);
    Log::writeFormatLog(Debug, "Heuristic Planning with {} states,", mData->size());

    do {
        clone_data(*mData, *mDataClone);
        for (const int stateIdx : *mBackupOrder) {
            Log::writeLog(std::format("Backing up time {}, state: {}", mdp.states[stateIdx]->time, stateIdx), LogLevel::Debug);
            backup(*mdp.states[stateIdx]);
            backups++;
            mExpanded->insert(stateIdx);
        }
        setPostOrderDFS();
#ifdef DEBUG
        Log::writeLog(std::format("Reached {} Backups at iteration {}", backups, iterations), LogLevel::Debug);
        Log::writeLog(std::format("Set Post Order DFS. Found {} state-times:", mFoundStates->size()), LogLevel::Debug);
        for (auto elem : *mFoundStates) { Log::writeLog(std::format("   t={}, s={};", mdp.states[elem]->time, elem), LogLevel::Trace); }
        Log::writeLog(std::format("\n"), LogLevel::Debug);
#endif
        iterations++;
    } while (checkForUnexpandedStates(mExpanded, mBackupOrder) or not checkConverged(*mData, *mDataClone));

    // For stats later
    this->expanded_states = mExpanded->size();
}

// Termination Condition
bool Solver::checkConverged(vector<vector<QValue>>& d, vector<vector<QValue>>& d_clone) {
    Log::writeLog("Checking converged states...", Debug);
    auto states = d.size();
    for (int s = 0; s < states; ++s) {
        if (d[s].size() != d_clone[s].size()) {
            Log::writeLog("No convergence. Size mismatch.", LogLevel::Debug);
            return false;
        }
        bool existsSimilar = false;
        for (QValue &qv : d[s] ) {
            for (QValue &qv2 : d_clone[s] ) {
                if (qv.isEquivalent(qv2)) {
                    existsSimilar = true;
                    break;
                }
            }
            if (existsSimilar) { break; }
        }
        if (!existsSimilar) {
            Log::writeFormatLog(Debug, "No Convergence detected on state {}.", s);
#ifdef DEBUG
            stringstream ss;
            ss << "Old:";
            for (QValue &qv : d_clone[s] ) { ss << qv.toString() << "  "; }
            ss << "\nNew:";
            for (QValue &qv : d[s] ) { ss << qv.toString() << "  "; }
            Log::writeLog(ss.str(), LogLevel::Trace);
#endif
            return false;
        }
    }Log::writeFormatLog(Debug, "Data converged!");
    return true;

}


void Solver::backup(State& state) {
    // Values we need
    vector<QValue> candidates = vector<QValue>();// The QValue candidates (corresponding to state-actions)
    vector<int> indicesOfUndominated = vector<int>();// Indices of candidate QValues that are undominated.
    vector<int> qValueIdxToAction = vector<int>();// Maps QValue index to action index.

    // Get the values
    getUnDomCandidates(state, candidates, indicesOfUndominated, qValueIdxToAction);
    if (indicesOfUndominated.empty()) { return; }
    // Set to Data
    mData->at(state.id).clear();
    for (auto elem : indicesOfUndominated) {
        mData->at(state.id).push_back(candidates[elem]);
    }
    mPi->at(state.id).clear();
    // Record what actions we're using.
    for (auto elem : indicesOfUndominated) {
        mPi->at(state.id).push_back(qValueIdxToAction[elem]);
    }
}

// Generate/load undominated sate-action values into candidates.
// Loads the indices of undominated candidates into indicesOfUndominated.
// Loads state-action indices into qValueIdxToAction.
void Solver::getUnDomCandidates(State& state, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction) {
    vector<shared_ptr<Action>> actions = *mdp.getActions(state);
    if (actions.size()==0 or state.time>=mdp.horizon) {
        return;
    }
    bool isStateLocked = mIsActionLock && mLockedActions.find(state.id) != mLockedActions.end();
    for (int aIdx = 0; aIdx < actions.size(); ++aIdx) {
        // If state is locked, not to this action, do not do a backup.
        if (isStateLocked && aIdx != mLockedActions[state.id]) {
            continue;
        }
        // Get successor QValues and initialise combinations space.
        vector<Successor*>* successors = MDP::getActionSuccessors(state, aIdx);
        gatherActionSuccessors(candidates, qValueIdxToAction, aIdx, successors);
    }
    // Find undominated candidates.
    pprune_alt(candidates, indicesOfUndominated);
    // GET RID OF DUPLICATES
#ifdef DEBUG
    stringstream ss;
    ss << "   Actions Undominated = ";
    for (auto elem : indicesOfUndominated) {
        auto a = actions.at(qValueIdxToAction[elem]);
        ss <<  (a->label) << " @ {" << candidates[elem].toString() << "}; ";
    }
    Log::writeLog(ss.str(), Trace);
#endif
}


void Solver::gatherActionSuccessors(vector<QValue>& candidates, vector<int>& qValueIdxToAction, int aIdx, vector<Successor*>* successors) {
    unordered_set<QValue,QValueHash,QValueEqual> uniqueCandidates;
    auto successorQValues = vector<vector<QValue>>();
    auto combos = vector<vector<QValue>>();
    for (auto & successor : *successors) {
        auto& q = mData->at(successor->target);
        successorQValues.push_back(q);
    }

    // Generate all combinations of Successor's QValues
    vector<QValue> currentcombo = vector<QValue>(successorQValues.size());
    generateCombinations(combos, successorQValues, currentcombo, 0);

   // Aggergate/gather for each consideration, for each combination.
    for (auto& elem : combos) {
        QValue new_qv = QValue(elem[0]);// Copy a random QValue (just for size really)
        for (int cIdx = 0; cIdx < mdp.considerations.size(); ++cIdx) {
            // Build successor's expectations -- comboExpects[i] is expected worth for successors[i].
            std::vector<WorthBase*> comboExpects = std::vector<WorthBase*>(successors->size());
            for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
                comboExpects[scrIdx] = elem[scrIdx].expectations[cIdx];
            }
            new_qv.expectations[cIdx] = mdp.considerations[cIdx]->gather(*successors, comboExpects, false);
        }
        // Store Candidate QValue and push back.
        uniqueCandidates.insert(new_qv);
    }
    for (auto& elem : uniqueCandidates) {
        candidates.push_back(elem);
        qValueIdxToAction.push_back(aIdx);
    }

}
void Solver::generateCombinations(vector<vector<QValue>>& allCombos, vector<vector<QValue>>& successorQVals, vector<QValue> &current, int scr_Idx) {
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
    if (inVector.size()==0) {return; }

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
    se.extractPolicies(policies, *mPi, *mBackupOrder);
}