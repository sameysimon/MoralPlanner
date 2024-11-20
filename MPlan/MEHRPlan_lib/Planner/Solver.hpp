#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "Environment/MDP.hpp"
#include "Solution.hpp"
#include "Policy.hpp"
#include <vector>
#include <unordered_set>
#include <set>

using namespace std;



class Solver {
    MDP& mdp;
    vector<vector<vector<QValue>>>* data;
    vector<vector<vector<int>>>* Pi;
    set<array<int, 2>, ArrayCompare>* Z;
    unordered_set<array<int, 2>, ArrayHash>* foundStates;// Explicitly encountered states

    void build_blank_data(vector<vector<vector<QValue>>>* d);

    bool checkForUnexpandedStates(unordered_set<array<int, 2>,ArrayHash>* expanded, set<array<int, 2>, ArrayCompare>* bpsg);
    void setPostOrderDFS(unordered_set<std::array<int, 2>, ArrayHash>* foundStates);
    bool PostOrderDFSCall(int stateIdx, int time, unordered_set<std::array<int, 2>, ArrayHash>& visited, unordered_set<std::array<int, 2>, ArrayHash>* foundStates);
    bool extractSolutionAtState(int stateIdx, int time, shared_ptr<Solution>& currSol, std::unordered_set<std::shared_ptr<Solution>,SolutionHash, SolutionEqual>& solSet);

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
    void backup(State& state, int time);
    void pprune_alt(std::vector<QValue>& inVector, std::vector<int>& outVector);
    void getCandidates(State& state, int time, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction);
    static bool checkConverged(vector<vector<vector<QValue>>>& data, vector<vector<vector<QValue>>>& data_other);
    vector<Policy*>* getSolutions();

    string SolutionSetToString(vector<shared_ptr<Solution>>& solSet);
    string SolutionSetToString(unordered_set<shared_ptr<Solution>, SolutionHash, SolutionEqual>& solSet);
};

#endif // SOLVER_HPP