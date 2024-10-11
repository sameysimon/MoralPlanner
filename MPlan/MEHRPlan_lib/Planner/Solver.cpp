//
//  Solver.cpp
//  MPlan
//
//  Created by Simon Kolker on 01/08/2024.
//  VI Multi Moral MDP (MM_MDP)
//
#include "Solver.hpp"
#include <iostream>
#include <sstream>
using namespace std;


// Single Objective Style Value Iteration.
Solution Solver::valueIteration() {
    Solution sol = Solution(mdp);
    Solution sol_copy = Solution(sol);
    int backups = 0;
    int iterations = 0;
    do {
        sol_copy = sol;
        // Select best and update
        for (int t = 0; t < mdp.horizon; t++) {
            for (int i = 0; i < mdp.total_states; i++) {
                getBestAction(sol, *mdp.states[i], t);
                backups++;
            }
            cout << "Time #" << t << endl;
            cout << sol.worthToString() << endl;
            cout << sol.policyToString();
            cout << endl;
        }
        if (backups % 1000 == 0) { cout << "Iteration #" << backups << endl; }
        iterations++;
    }
    while (sol.equivalencyCheck(sol_copy)==false);
    cout << sol.worthToString() << endl;
    cout << sol.policyToString();
    cout << "Done! at " << backups << " backups and " << iterations << " iterations." << endl << endl << endl;
    return sol;
}

void Solver::getBestAction(Solution& sol, State& state, int time) {
    vector<Action*>* actions = mdp.getActions(state, time);
    if (actions->size()==0) {
        return;
    }
    // Collect Q-Value for every Action
    vector<QValue> qvals = vector<QValue>();
    for (int actionIdx = 0; actionIdx < actions->size(); ++actionIdx) {
        vector<Successor*>* successors = mdp.getActionSuccessors(state, actionIdx);
        QValue qv = sol.vectorGather(*successors, time);// TODO I think this copies the QValue, which works but is slow?
        qvals.push_back(qv);
    }

    // Get Best Q-Value for every action.
    int bestQIdx = 0;
    for (int i=1; i < qvals.size(); i++) {
        if (mdp.compareQValues(qvals[bestQIdx], qvals[i]) == -1) {
            bestQIdx = i;
        }
    }

    // kind of janky but easy.
    sol.setToQValue(state, time, qvals[bestQIdx]);// TODO may not work right, needs overhaul tbh.
    sol.setAction(state, time, bestQIdx);

}





/*

bool Solver::consistencyCheck(std::vector<Solution> &solSet) {
    for (Solution sol : solSet) {
        // Create new solution from current one.
        // Maybe need a better copy method than create new. Seems wasteful.
        Solution solPrime = Solution(mdp);
        solPrime.copyFrom(sol);
        
        
        //this->backup(sol); //Unfinished
        
       // if (not sol.equivalencyCheck(solPrime)) {
       //     return false;
        //}
    }
    return true;
}




//
// MAIN ENTRY TO SOLVER
//
Solution Solver::solve(MDP &mdp) {
     Will contain the moving undomintated set of solutions.
     std::vector<Solution> solSet = std::vector<Solution>();
     solSet.reserve(10);

     // Create initial solution, initials it to heuristics.
     solSet[0] = Solution(mdp);
     solSet[0].setAllToHeuristic(mdp);

     std::unordered_set<State*> expanded = std::unordered_set<State*>();
     solSet.reserve(mdp.total_states*0.33); // Reserve enough for a third of the state space.

     while (not this->consistencyCheck(solSet, mdp)) {
         while (not isFullyExpanded(expanded, mdp)) {
             std::vector<State*> postOrder = this->getPostOrderDFS(solSet, mdp);
             for (State* s : postOrder) {
                 //this->backup(s, solSet);// uses all theories + cost fn.
                 // Possibly getActions/update solSet or something... Or get sol/backup to handle it.
                 if (expanded.find(s) == expanded.end())
                     expanded.insert(s);


             }
         }
     }

     solSet is now undominated set of solutions to the MDP.
     Now must find do the MEHR between solSet.
     1. Find all outcomes from each sol.
     2. Find value of each outcome, by each theory.
     3. Compare different sol outcomes pairwise, for each theory (not cost-fn).
     4. If outcome-value greater and sol-value greater, form attack.
     5. Add non-acceptability from attacks.
     6. Find all equally acceptable policies.
     7. Return policy with minimum cost.
    Solution sol = Solution(mdp);
    return sol;
}


std::vector<State*> Solver::getPostOrderDFS(std::vector<Solution> &solSet, MDP &mdp) {
    // Depth-first-search, gathering states.
    std::vector<State*> ordered = std::vector<State*>();



    return ordered;
}

void backup(State* state, MDP& mdp, std::vector<Solution> &solSet) {
    // Repeat all for every solution...
    for (Solution sol : solSet) {
        std::vector<Action*> actions = mdp.getActions(*state);
        //std::vector<std::unique_ptr<QValue>()> qValues;
        for (Action* a : actions) {
            //qValues.push_back(sol.gather(*state, 0, *a));
        }
        
        // GET BEST Q-VALUE
        
        // UPDATE EACH SOL TO USE ACTIO FOR BEST Q-VALUE.
    }
    
    
    
    
}
*/

