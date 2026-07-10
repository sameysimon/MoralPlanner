#pragma once


#include "Environment/MDP.hpp"
#include "Policy.hpp"
#include <vector>
#include <unordered_set>


using namespace std;

struct Candidate {
    QValue qv;
    int action = -1;
};

class Solver {
    MDP& mdp;
    // Worth vector set Function. It maps state to a list of solutions' possible QValues.
    vector<vector<QValue>> mData;
    vector<vector<QValue>>* pTempData = nullptr;

    vector<int> mBackupOrder;
    unique_ptr<unordered_set<int>> mFoundStates;// Explicitly encountered states
    unordered_set<int> mExpanded;

    bool mIsActionLock = false;
    unordered_map<size_t, size_t> mLockedActions;

    // Used in the backup function
    vector<QValue> candidates = vector<QValue>();// The QValue candidates (corresponding to state-actions)
    vector<int> indicesOfUndominated = vector<int>();// Indices of candidate QValues that are undominated.
    vector<int> qValueIdxToAction = vector<int>();// Maps QValue index to action index.


    bool checkForUnexpandedStates(unordered_set<int>& expanded, vector<int>& bpsg);
    bool PostOrderDFSCall(int stateIdx, int time, unordered_set<int>& visited, unique_ptr<unordered_set<int>>& foundStates);

public:
    // Multi-policy. Maps state
    vector<vector<int>> mPi;

    size_t expanded_states=0;
    int expansions=0;
    int backups=0;

    explicit Solver(MDP& _mdp, bool use_domain_heuristic) : mdp(_mdp) {
        // Initialise Solver data structures
        mData = build_blank_data(use_domain_heuristic);

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
    vector<vector<QValue>> build_blank_data(bool use_domain_heuristic=false, size_t time=0);

    // Single-Objective VI (for heuristics)
    void BuildIndependentHeuristic();
    void SCVI(size_t con_idx, vector<size_t> &statesByTime);
    void MCDP();

    vector<QValue> EvaluatePolicy(Policy& pi, size_t until_time);


    // Multi-objective IAO*
    void setPostOrderDFS();
    void MC_iAO_Star();
    void backup(State& state);
    void backupTwo(State& state);

    void gatherPFActionWorth(list<Candidate>& candidates, vector<Successor*>* successors, int aIdx);
    void gatherActionSuccessors(vector<QValue>& candidates, vector<int>& qValueIdxToAction, int aIdx,
                                vector<Successor*>* successors);


    template <typename T, typename Accessor>
    bool ParetoFilter(MDP& mdp, std::list<T>& pfVector, T&& new_qv, Accessor getQValue) {
        const QValue& newValue = getQValue(new_qv);
        if (!mdp.isQValueInBudget(newValue)) {
            return false;
        }
        auto it = pfVector.begin();
        while (it != pfVector.end()) {
            const QValue& existingValue = getQValue(*it);
            if (existingValue.isEquivalent(newValue)) {
                return false;
            }
            int r = mdp.ParetoCompare(newValue, existingValue);
            if (r == -1) {
                return false;
            }
            if (r == 1) {
                it = pfVector.erase(it);
            } else {
                ++it;
            }
        }

        pfVector.push_front(std::forward<T>(new_qv));
        return true;
    }

    static std::vector<int> Pprune(MDP& mdp, std::vector<QValue>& inVector);
    static void Pprune(MDP&mdp, std::vector<QValue>& inVector, std::vector<int>& outVector);

    template <class T, class Accessor>
    static std::vector<int> Pprune(MDP& mdp, const T& inVector, Accessor getQValue);

    template <class T, class Accessor>
    static void Pprune(MDP& mdp, const T& inVector, std::vector<int>& outVector, Accessor getQValue);

    template <class T, class Accessor>
    static void Pprune(MDP& mdp, T& inVector, std::vector<int>& outVector, Accessor& getQValue);

    static bool CheckProperFront(MDP& mdp, vector<QValue*> &ExistingWorth, QValue& newWorth) {
        if (!mdp.isQValueInBudget(newWorth)) {
            return false;
        }
        for (auto qv : ExistingWorth) {
            int r = mdp.ParetoCompare(*qv, newWorth);
            if (r == 1) {
                return false;
            }
        }
        return true;
    }


    vector<vector<QValue*>> GetSuccessorQValueCombinations(vector<Successor*>* successors);
    vector<QValue>& GetQValuesAtState(size_t stateIdx) {
        return mData.at(stateIdx);
    }
    void getUnDomCandidates(State& state, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction);
    static bool checkConverged(vector<vector<QValue>>& d, vector<vector<QValue>>& d_clone);

    void ResetExpansion() {
        mExpanded.clear();
        mPi = vector(mdp.states.size(), vector<int>());

    }
    void UseTempData(bool doUseTempData = true) {
        if (doUseTempData) {
            // cache the current data
            pTempData = new vector<vector<QValue>>();
            *pTempData = mData;
            return;
        }
        mData = *pTempData;
        delete pTempData;
    }
    void lockPolicy(Policy& policy) {
        mIsActionLock = true;
        for (auto x : policy.policy ) {
            mLockedActions[x.first] = x.second;
        }
    }
    void lockAction(State& state, size_t aIdx, Policy& factPolicy) {
        // Bool tells solver that locked actions should not be optimised/actions should not change.
        mIsActionLock = true;
        // Lock the state-action.
        mLockedActions[state.id] = aIdx;
        // Lock all ancestors (with time less than passed state)
        for (auto* s : mdp.states) {
            if (s->time < state.time) {
                if (auto state_actIdx = factPolicy.getAction(s->id)) {
                    mLockedActions[s->id] = state_actIdx.value();
                }

            }
        }
    }
    void removeLocks() {
        mLockedActions.clear();
        mIsActionLock = false;
    }
};

