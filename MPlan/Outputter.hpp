//
// Created by Simon Kolker on 03/10/2024.
//

#ifndef OUTPUTTER_HPP
#define OUTPUTTER_HPP

#include "MDP.hpp"
#include <nlohmann/json.hpp>
#include "Solver.hpp"
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using namespace std;


class Outputter {
    public:
    std::vector<size_t> static sort_indices(const std::vector<double>& non_accept) {
        // Create a vector of indices (0, 1, 2, ..., non_accept.size() - 1)
        std::vector<size_t> indices(non_accept.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }
        // Sort indices based on comparing the values in non_accept
        std::sort(indices.begin(), indices.end(),
                  [&non_accept](size_t i1, size_t i2) { return non_accept[i1] < non_accept[i2]; });

        return indices;
    }


    void static outputResults(MDP* mdp, vector<double>* non_accept, long long durationPlan, long long durationMEHR, vector<shared_ptr<Solution>>& candidates, std::string filePath) {
        json results;// Overall file

        results["duration_Plan"] = durationPlan;
        results["duration_MEHR"] = durationMEHR;
        results["duration_total"] = durationMEHR + durationPlan;

        json solutions = json::array();
        std::vector<size_t> sorted_indices = sort_indices(*non_accept);
        int counter = 0;
        for (auto i : sorted_indices) {
            json solution_json;
            solution_json["Non-Acceptability"] = (*non_accept)[i];
            if (mdp->non_moralTheoryIdx!=-1) {
                solution_json["Expected Cost"] = candidates[i]->expecters[mdp->non_moralTheoryIdx]->expectations[0][0]->ToString();
            }

            auto tree = DFS_Search(*mdp, candidates[i]);
            json actions;
            for (auto elem : *tree) {
                std::ostringstream oss;
                oss << elem[0] << ", " << elem[1];
                actions[oss.str()] =  candidates[i]->policy[elem[0]][elem[1]];
            }
            solution_json["Action_Map"] = actions;

            solutions.push_back(solution_json);
            results["solutions"] = solutions;
            // TODO Give info for state-0 time-0 for all.
            // TODO Give Policy for sure with estimates along the way?
            std::cout << counter << std::endl;
            std::cout << "Non-Acceptability=" << (*non_accept)[i] << std::endl;
            std::cout <<  (candidates)[i]->worthToString() << std::endl;
            std::cout <<  (candidates)[i]->policyToString() << std::endl << std::endl;


            counter++;
        }

        //
        // Save File
        //
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << results.dump(4);
            file.close();
            std::cout << "JSON file created successfully!" << std::endl;
        } else {
            std::cerr << "Could not open the file for writing!" << std::endl;
        }
    }


    bool static DFS_Search_Call(MDP& mdp, int stateIdx, int time, unordered_set<std::array<int, 2>, ArrayHash>& visited, set<array<int, 2>,ArrayCompare>* tree, std::shared_ptr<Solution>& sol) {
        if (tree->find({time, stateIdx}) != tree->end()) {
            return true; // If already visited, don't search.
        }
        if (time >= mdp.horizon) {
            return false;// If post-horizon, don't search.
        }
        tree->insert({time,stateIdx});

        int stateAction = sol->policy[time][stateIdx];
        if (stateAction==-1) { return true; }
        auto scrs = mdp.getActionSuccessors(*mdp.states[stateIdx], stateAction);
        if (scrs==nullptr) { return true; }
        for (auto scr : *scrs) {
            int ttime = time+1;// CHANGED TIME HERE
            DFS_Search_Call(mdp, scr->target, ttime, visited, tree, sol);
        }
        return true;
    }

    set<array<int, 2>,ArrayCompare> static *DFS_Search(MDP& mdp, std::shared_ptr<Solution>& sol) {
        auto tree = new set<array<int, 2>,ArrayCompare>();
        unordered_set<std::array<int, 2>, ArrayHash> visited = unordered_set<std::array<int, 2>, ArrayHash>();
        DFS_Search_Call(mdp, 0, 0, visited, tree, sol);
        return tree;
    }


};



#endif //OUTPUTTER_HPP
