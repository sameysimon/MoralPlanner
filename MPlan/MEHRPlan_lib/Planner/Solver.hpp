#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "Environment/MDP.hpp"
#include "Policy.hpp"
#include <vector>
#include <unordered_set>
#include <set>

using namespace std;

class Solver {
    MDP& mdp;
    vector<vector<QValue>>* mData;
    vector<vector<QValue>>* mDataClone;
    vector<vector<int>>* mPi;
    vector<int>* mBackupOrder;
    unique_ptr<unordered_set<int>> mFoundStates;// Explicitly encountered states
    unordered_set<int>* mExpanded;

    bool mIsActionLock = false;
    unordered_map<size_t, size_t> mLockedActions;

    void build_blank_data(vector<vector<QValue>>* d);
    bool checkForUnexpandedStates(unordered_set<int>* expanded, vector<int>* bpsg);
    bool PostOrderDFSCall(int stateIdx, int time, unordered_set<int>& visited, unique_ptr<unordered_set<int>>& foundStates);
    bool hasConsiderationConverged(size_t con_idx, vector<vector<QValue>>& d, vector<vector<QValue>>& d_) const;
    void clone_data(vector<vector<QValue>>& d, vector<vector<QValue>>& d_);

public:
    size_t expanded_states=0;
    long explicit_states=0;
    int iterations=0;
    int backups=0;

    explicit Solver(MDP& _mdp) : mdp(_mdp) {
        // Initialise Multi-Worth Function. It maps state to a list of solutions' possible QValues.
        mData = new vector<vector<QValue>>();
        build_blank_data(mData);
        // Initialise clone.
        mDataClone = new vector<vector<QValue>>();
        build_blank_data(mDataClone);


        // Initialise Pi. It maps state to a vector of available actions.
        mPi = new vector<vector<int>>();
        for (int s = 0; s < mdp.states.size(); ++s) {
            mPi->emplace_back();
        }

        backups = 0;
        iterations = 0;

        // Explicitly encountered states
        mFoundStates = make_unique<unordered_set<int>>();
        mFoundStates->insert(0);

        mExpanded = new unordered_set<int>();

        mBackupOrder = new vector<int>(); // Ordered Best partial sub-graph.
        mBackupOrder->emplace_back(0);
    }

    ~Solver() {
        delete mData;
        delete mDataClone;
        delete mBackupOrder;
        delete mExpanded;
        delete mPi;
    }
    vector<vector<QValue>>* getData() {
        return mData;
    }
    bool operator==(const Solver& other) const {
        if (mData->size() != other.mData->size()) {
            return false;
        }
        if (mBackupOrder->size() != other.mBackupOrder->size()) {
            return false;
        }
        for (int i = 0; i < mData->size(); ++i) {
            if (mData->at(i).size() != other.mData->at(i).size()) {
                return false;
            }
            for (int j = 0; j < mData->at(i).size(); ++j) {
                if (false==(mData->at(i)[j] == other.mData->at(i)[j])) {
                    return false;
                }
            }
        }
        return true;
    }

    // Single-Objective VI (for heuristics)
    void BuildIndependentHeuristic();
    void SOVI(size_t con_idx, vector<size_t> &statesByTime);

    // Multi-objective IAO*
    void setPostOrderDFS();
    void MOiAO_Star();
    void backup(State& state);
    void gatherActionSuccessors(vector<QValue>& candidates, vector<int>& qValueIdxToAction, int aIdx,
                                vector<Successor*>* successors);
    void pprune_alt(std::vector<QValue>& inVector, std::vector<int>& outVector);
    void generateCombinations(vector<vector<QValue>>& allCombos, vector<vector<QValue>>& successorQVals, vector<QValue> &current, int scr_Idx);
    void getUnDomCandidates(State& state, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction);
    bool checkConverged(vector<vector<QValue>>& d, vector<vector<QValue>>& d_clone);
    void getSolutions(vector<Policy*>& policies);

    void lockAction(State& state, size_t aIdx, Policy& factPolicy) {
        // Bool tells solver that locked actions should not be optimised/actions should not change.
        mIsActionLock = true;
        // Lock the state-action.
        mLockedActions[state.id] = aIdx;
        // Lock all ancestors (with time less than passed state)
        for (auto* s : mdp.states) {
            if (s->time < state.time) {
                mLockedActions[s->id] = factPolicy.policy.at(s->id);
            }
        }
    }
    void removeLocks() {
        mLockedActions.clear();
        mIsActionLock = false;
    }
};

#endif // SOLVER_HPP