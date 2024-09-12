#include "Environment/MDP.hpp"
#include "Solution.hpp"
#include <vector>
#include <unordered_set>

class Solver {
    MDP& mdp;
    // static bool consistencyCheck(std::vector<Solution> &solSet);
    // bool isFullyExpanded(std::unordered_set<State*> expanded, MDP &mdp) {return false;}
    // std::vector<State*> getPostOrderDFS(std::vector<Solution> &solSet, MDP &mdp); // Will need best actions according to solSet
    Action* getBestAction(Solution& sol, State& state, int time, bool setQValue);
    void newSolutionFrom(Solution& sol, State& state, int time, Action* action, int stateActionIdx, Solution& newSol);
    void backupSolutions(std::vector<Solution*>& solSet, State& s, int time, std::vector<Solution*>& newSols);
    void backup(std::vector<std::shared_ptr<Solution>>& solSet, State& state, int time);
public:
    Solver(MDP& _mdp) : mdp(_mdp) {}
    Solution valueIteration();
    std::vector<std::shared_ptr<Solution>> MOValueIteration();
    void pprune(std::vector<QValue>& inSet, std::vector<int>& outSet);
    void pprune(std::vector<Solution*>& inSet, std::vector<Solution*>& outSet, State& state, int time);
    std::string SolutionSetToString(std::vector<std::shared_ptr<Solution>>& solSet);
};