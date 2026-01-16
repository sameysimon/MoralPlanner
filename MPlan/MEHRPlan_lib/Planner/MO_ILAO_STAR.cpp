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
void Solver::build_blank_data(vector<vector<QValue>> &d) {
    d.reserve(mdp.states.size());
    for (auto & state : mdp.states) {
        auto us = vector<QValue>(1);
        // New QValue, fill with heuristics
        QValue qv = QValue(mdp.considerations.size());
        for (auto c : mdp.considerations) {
            qv.expectations.push_back(c->newHeuristic(*state));
        }
        us[0] = qv;
        d.push_back(std::move(us));
    }
}

//
// MAIN ALGORITHM.
//
void Solver::MC_iAO_Star() {
    do {
        expansions++;
        for (const int stateIdx : mBackupOrder) {
            backup(*mdp.states[stateIdx]);
            backups++;
            mExpanded.insert(stateIdx);
        }
        setPostOrderDFS();
#ifdef DEBUG
        Log::writeFormatLog(LogLevel::Debug, "Reached {} Backups at iteration {}", backups, expansions);
        Log::writeFormatLog(LogLevel::Debug, "Set Post Order DFS. Found {} state-times:", mFoundStates->size());
        for (auto elem : *mFoundStates) { Log::writeFormatLog(LogLevel::Trace, "   t={}, s={};", mdp.states[elem]->time, elem); }
        Log::writeLog("\n", LogLevel::Debug);
#endif
    } while (checkForUnexpandedStates(mExpanded, mBackupOrder));
    // For stats later
    this->expanded_states = mExpanded.size();
}

void Solver::backup(State& state) {
    // Operations on following
    candidates.clear();
    indicesOfUndominated.clear();
    qValueIdxToAction.clear();

    // Get the values
    getUnDomCandidates(state, candidates, indicesOfUndominated, qValueIdxToAction);
    if (indicesOfUndominated.empty()) { return; }

    // Update Data values to current undominated.
    mData.at(state.id).clear();
    for (auto elem : indicesOfUndominated) {
        mData.at(state.id).push_back(candidates[elem]);
    }
    // Update Action map to current undominated.
    mPi.at(state.id).clear();
    // Record what actions we're using.
    for (auto elem : indicesOfUndominated) {
        mPi.at(state.id).push_back(qValueIdxToAction[elem]);
    }
}

// Generate/load undominated sate-action values into candidates.
// Loads the indices of undominated candidates into indicesOfUndominated.
// Loads state-action indices into qValueIdxToAction.
void Solver::getUnDomCandidates(State& state, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction) {
    vector<shared_ptr<Action>> actions = *mdp.getActions(state);
    if (actions.size()==0 || state.time>=mdp.horizon) {
        return;
    }
    bool isStateLocked = mIsActionLock && mLockedActions.find(state.id) != mLockedActions.end();
    // Populate candidates (and other param vectors) with action aggregations
    for (int aIdx = 0; aIdx < actions.size(); ++aIdx) {
        // If state is locked, not to this action, skip backup.
        if (isStateLocked && aIdx != mLockedActions[state.id]) {
            continue;
        }
        // Get successor QValues and initialise combinations space.
        vector<Successor*>* successors = MDP::getActionSuccessors(state, aIdx);
        gatherActionSuccessors(candidates, qValueIdxToAction, aIdx, successors);
    }
    pprune(candidates, indicesOfUndominated);

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
    unordered_set<QValue, QValueHash, QValueEqual> uniqueCandidates;
    // Generate all combinations of Successor's QValues
    vector<vector<QValue*>> combos = GetSuccessorQValueCombinations(successors);

   // Aggergate/gather for each consideration, for each combination.
    for (auto& elem : combos) {
        // TODO can make new_qv a pointer to save copies...
        QValue new_qv = QValue(mdp);
        for (int cIdx = 0; cIdx < mdp.considerations.size(); ++cIdx) {
            // Build successor's expectations -- comboExpects[i] is expected worth for successors[i].
            std::vector<WorthBase*> comboExpects = std::vector<WorthBase*>(successors->size());
            for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
                comboExpects[scrIdx] = elem[scrIdx]->expectations[cIdx].get();
            }
            new_qv.expectations[cIdx] = mdp.considerations[cIdx]->gather(*successors, comboExpects, false);
        }
        // Store Candidate QValue and push back.
        uniqueCandidates.insert(new_qv);
    }
    // Convert unique set to vector
    for (auto& new_qv : uniqueCandidates) {
        candidates.push_back(new_qv);
        qValueIdxToAction.push_back(aIdx);
    }
}

vector<vector<QValue*>> Solver::GetSuccessorQValueCombinations(vector<Successor*>* successors) {
    vector<vector<QValue*>> combs(1);
    for (auto& scr : *successors) {
        vector<vector<QValue*>> next;
        for (const auto& partial : combs) {
            for (auto& w : mData[scr->target]) {
                auto combo = partial;
                combo.push_back(&w);
                next.push_back(std::move(combo));
            }
        }
        combs = std::move(next);
    }
    return combs;
}

void Solver::pprune(std::vector<QValue>& inVector, std::vector<int>& outVector) {
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
            int r = mdp.ParetoCompare(qv, inVector[j]);
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

bool Solver::checkForUnexpandedStates(unordered_set<int>& expanded, vector<int>& bpsg) {
    if (expanded.size()==0 or bpsg.size()==0) {
        return true;
    }
    bool unexpanded = false;
    for (const auto elem : bpsg) {
        if (mdp.non_moralTheoryIdx!= -1 && mdp.states[elem]->isGoal) {
            continue;
        }
        if (expanded.find(elem) == expanded.end()) {
            return true;
        }
    }
    return unexpanded;
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
            cout << "\n\n **** No Convergence detected on state. **** \n\n " << endl;
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


void Solver::getSolutions(vector<unique_ptr<Policy>> &policies){
    auto se = SolutionExtracter(mdp);
    se.Extract(policies, mPi, mBackupOrder);
}