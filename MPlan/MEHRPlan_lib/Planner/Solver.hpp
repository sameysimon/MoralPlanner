#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "Environment/MDP.hpp"
#include "Solution.hpp"
#include <vector>
#include <unordered_set>
#include <set>

using namespace std;

struct ArrayHash {
    size_t operator()(const array<int, 2>& arr) const {
        return hash<int>()(arr[0]) ^ hash<int>()(arr[1]);
    }
};
struct ArrayCompare {
    bool operator()(const array<int, 2>& lhs, const array<int, 2>& rhs) const {
        if (lhs[0] != rhs[0]) {
            return lhs[0] > rhs[0]; // Sort in descending order by time
        }
        return lhs[1] > rhs[1]; // Otherwise sort by state.
    }
};
struct SetArrayHash {
    size_t operator()(const set<array<int, 2>, ArrayCompare>& s) const {
        size_t hash_value = 0;
        for (const auto& arr : s) {
            hash_value ^= ArrayHash()(arr);  // Combine hashes of the arrays
        }
        return hash_value;
    }
};

class Solver {
    MDP& mdp;
    vector<vector<unordered_set<QValue, QValueHash, QValueEqual>>>* data;
    vector<vector<unordered_set<int>>>* Pi;
    set<array<int, 2>, ArrayCompare>* Z;
    unordered_set<array<int, 2>, ArrayHash>* foundStates;// Explicitly encountered states

    void build_blank_data(vector<vector<unordered_set<QValue,QValueHash,QValueEqual>>>* d);

    bool checkForUnexpandedStates(unordered_set<array<int, 2>,ArrayHash>* expanded, set<array<int, 2>, ArrayCompare>* bpsg);
    void setPostOrderDFS(unordered_set<std::array<int, 2>, ArrayHash>* foundStates);
    bool PostOrderDFSCall(int stateIdx, int time, unordered_set<std::array<int, 2>, ArrayHash>& visited, unordered_set<std::array<int, 2>, ArrayHash>* foundStates);

    bool solutionRecurse(int time, int stateIdx, shared_ptr<Solution> currSol, unordered_set<shared_ptr<Solution>, SolutionHash, SolutionCompare>& solSet);
    void getCandidates(State& state, int time, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction);
public:
    size_t expanded_states=0;
    long explicit_states=0;
    int iterations=0;
    int backups=0;


    Solver(MDP& _mdp) : mdp(_mdp) {}
    ~Solver() {
        delete foundStates;
        delete data;
        delete Z;
    }
    // Single Objective
    Solution valueIteration();
    void getBestAction(Solution& sol, State& state, int time);

    // Multi-objective ILAO*
    void MOiLAO();
    vector<shared_ptr<Solution>>* extractSolutions();
    void backup(State& state, int time);
    void pprune_alt(std::vector<QValue>& inVector, std::vector<int>& outVector);

    string SolutionSetToString(vector<shared_ptr<Solution>>& solSet);
    string SolutionSetToString(unordered_set<shared_ptr<Solution>, SolutionHash, SolutionCompare>& solSet);
};

#endif // SOLVER_HPP