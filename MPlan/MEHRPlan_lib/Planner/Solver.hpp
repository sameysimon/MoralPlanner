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
    vector<vector<QValue>>* data;
    vector<vector<int>>* Pi;
    vector<int>* Z;
    unique_ptr<unordered_set<int>> foundStates;// Explicitly encountered states


    void build_blank_data(vector<vector<QValue>>* d);

    bool checkForUnexpandedStates(unordered_set<int>* expanded, vector<int>* bpsg);
    void setPostOrderDFS(unique_ptr<unordered_set<int>>& foundStates);
    bool PostOrderDFSCall(int stateIdx, int time, unordered_set<int>& visited, unique_ptr<unordered_set<int>>& foundStates);

public:
    size_t expanded_states=0;
    long explicit_states=0;
    int iterations=0;
    int backups=0;


    Solver(MDP& _mdp) : mdp(_mdp) {}
    ~Solver() {
        delete data;
        delete Z;
    }

    // Multi-objective ILAO*
    void MOiLAO();
    void backup(State& state, int time);
    void pprune_alt(std::vector<QValue>& inVector, std::vector<int>& outVector);
    void getCandidates(State& state, int time, vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction);
    bool checkConverged(vector<vector<QValue>>& data, vector<vector<QValue>>& data_other);
    void getSolutions(vector<Policy*>& policies);

    bool operator==(const Solver& other) const {
        if (data->size() != other.data->size()) {
            return false;
        }
        if (Z->size() != other.Z->size()) {
            return false;
        }
        for (int i = 0; i < data->size(); ++i) {
            if (data->at(i).size() != other.data->at(i).size()) {
                return false;
            }
            for (int j = 0; j < data->at(i).size(); ++j) {
                if (false==(data->at(i)[j] == other.data->at(i)[j])) {
                    return false;
                }
            }
        }
        return true;


    }
};

#endif // SOLVER_HPP