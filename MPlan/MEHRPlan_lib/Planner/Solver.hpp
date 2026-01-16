#pragma once


#include "Environment/MDP.hpp"
#include "Policy.hpp"
#include <vector>
#include <unordered_set>


using namespace std;

class Solver {
    MDP& mdp;
    // Worth vector set Function. It maps state to a list of solutions' possible QValues.
    vector<vector<QValue>> mData;
    // Multi-policy. Maps state
    vector<vector<int>> mPi;
    vector<int> mBackupOrder;
    unique_ptr<unordered_set<int>> mFoundStates;// Explicitly encountered states
    unordered_set<int> mExpanded;

    bool mIsActionLock = false;
    unordered_map<size_t, size_t> mLockedActions;

    // Used in the backup function
    vector<QValue> candidates = vector<QValue>();// The QValue candidates (corresponding to state-actions)
    vector<int> indicesOfUndominated = vector<int>();// Indices of candidate QValues that are undominated.
    vector<int> qValueIdxToAction = vector<int>();// Maps QValue index to action index.

    void build_blank_data(vector<vector<QValue>>& d);
    bool checkForUnexpandedStates(unordered_set<int>& expanded, vector<int>& bpsg);
    bool PostOrderDFSCall(int stateIdx, int time, unordered_set<int>& visited, unique_ptr<unordered_set<int>>& foundStates);

public:
    size_t expanded_states=0;
    long explicit_states=0;
    int expansions=0;
    int backups=0;

    explicit Solver(MDP& _mdp) : mdp(_mdp) {
        // Initialise Solver data structures
        mData = vector(mdp.states.size(), vector(2, QValue(mdp)));
        build_blank_data(mData);

        mPi = vector(mdp.states.size(), vector<int>());

        // Explicitly encountered states
        mFoundStates = make_unique<unordered_set<int>>();
        mFoundStates->insert(0);

        mExpanded = unordered_set<int>();

        mBackupOrder = vector<int>(); // Ordered Best partial sub-graph.
        mBackupOrder.emplace_back(0);

        backups = 0;
        expansions = 0;

        candidates.reserve(mdp.actions.size());
        indicesOfUndominated.reserve(mdp.actions.size());
        qValueIdxToAction.reserve(mdp.actions.size());
    }

    ~Solver() = default;

    bool operator==(const Solver& other) const {
        if (mData.size() != other.mData.size()) {
            return false;
        }
        if (mBackupOrder.size() != other.mBackupOrder.size()) {
            return false;
        }
        for (int i = 0; i < mData.size(); ++i) {
            if (mData.at(i).size() != other.mData.at(i).size()) {
                return false;
            }
            for (int j = 0; j < mData.at(i).size(); ++j) {
                if (false==(mData.at(i)[j] == other.mData.at(i)[j])) {
                    return false;
                }
            }
        }
        return true;
    }

    // Single-Objective VI (for heuristics)
    void BuildIndependentHeuristic();
    void SCVI(size_t con_idx, vector<size_t> &statesByTime);

    // Multi-objective IAO*
    void setPostOrderDFS();
    void MC_iAO_Star();
    void backup(State& state);
    void gatherActionSuccessors(vector<QValue>& candidates, vector<int>& qValueIdxToAction, int aIdx,
                                vector<Successor*>* successors);
    void pprune(std::vector<QValue>& inVector, std::vector<int>& outVector);
    vector<vector<QValue*>> GetSuccessorQValueCombinations(vector<Successor*>* successors);
    void getUnDomCandidates(State& state, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction);
    static bool checkConverged(vector<vector<QValue>>& d, vector<vector<QValue>>& d_clone);
    void getSolutions(vector<unique_ptr<Policy>>& policies);

    void lockAction(State& state, size_t aIdx, Policy& factPolicy) {
        // Bool tells solver that locked actions should not be optimised/actions should not change.
        mIsActionLock = true;
        // Lock the state-action.
        mLockedActions[state.id] = aIdx;
        // Lock all ancestors (with time less than passed state)
        for (auto* s : mdp.states) {
            if (s->time < state.time) {
                mLockedActions[s->id] = factPolicy.getAction(s->id);
            }
        }
    }
    void removeLocks() {
        mLockedActions.clear();
        mIsActionLock = false;
    }
};

