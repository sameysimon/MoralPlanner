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
        } else {
            return lhs[1] > rhs[1]; // Otherwise sort by state.
        }
    }
};

class Solver {
    MDP& mdp;
    // static bool consistencyCheck(vector<Solution> &solSet);
    // bool isFullyExpanded(unordered_set<State*> expanded, MDP &mdp) {return false;}
    // vector<State*> getPostOrderDFS(vector<Solution> &solSet, MDP &mdp); // Will need best actions according to solSet
    void copySolSets(vector<shared_ptr<Solution>>& solSet, vector<shared_ptr<Solution>>& solSetCopy);
    Action* getBestAction(Solution& sol, State& state, int time, bool setQValue);
    void newSolutionFrom(Solution& sol, State& state, int time, Action* action, int stateActionIdx, Solution& newSol);
    void backupSolutions(vector<Solution*>& solSet, State& s, int time, vector<Solution*>& newSols);
    void backup(vector<shared_ptr<Solution>>& solSet, State& state, int time);
    bool hasConverged(vector<shared_ptr<Solution>>& solSet, vector<shared_ptr<Solution>>& solSet_);
    bool checkForUnexpandedStates(unordered_set<array<int, 2>,ArrayHash>* expandedStates, unordered_set<array<int, 2>,ArrayHash>* bpsg);
    void setPostOrderDFS(set<array<int, 2>, ArrayCompare>* bpsg, unordered_set<array<int, 2>, ArrayHash>* internal, vector<shared_ptr<Solution>>& solSet);
    bool PostOrderDFSCall(int stateIdx, int time, unordered_set<std::array<int, 2>, ArrayHash>& visited, set<array<int, 2>, ArrayCompare>* bpsg, unordered_set<std::array<int, 2>, ArrayHash>* internal, vector<std::shared_ptr<Solution>>& solSet);
public:
    Solver(MDP& _mdp) : mdp(_mdp) {}
    Solution valueIteration();
    vector<shared_ptr<Solution>> MOValueIteration();
    vector<shared_ptr<Solution>> MOiLAO();

    void pprune(vector<QValue>& inSet, vector<int>& outSet);
    void pprune(vector<Solution*>& inSet, vector<Solution*>& outSet, State& state, int time);
    string SolutionSetToString(vector<shared_ptr<Solution>>& solSet);
};