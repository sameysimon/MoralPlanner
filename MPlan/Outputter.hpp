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


    void static outputResults(MDP* mdp, vector<double>* non_accept, vector<long long> &durations, vector<Policy*>& policies, const std::string& fileOut, const std::string& fileIn, Solver& solver) {
        json results;// Overall file


        results["Expanded"] = solver.expanded_states;
        results["Backups"] = solver.backups;
        results["Iterations"] = solver.iterations;
        results["SolutionTotal"] = policies.size();
        /*for (int rank=0; rank < mdp->groupedTheoryIndices.size(); rank++) {
            for (int thIdx=0; thIdx < mdp->groupedTheoryIndices[rank].size(); thIdx++) {
                results["Theories"].push_back(to_string(rank));
                results["Theories"].push_back(mdp->theories[mdp->groupedTheoryIndices[rank][thIdx]]->label);
            }
        }*/

        results["duration_Plan"] = durations[0];
        results["duration_MEHR"] = durations[1];
        long long x = std::accumulate(durations.begin(), durations.end(), (long long)(0)) ;
        results["duration_total"] = x;

        json solutions = json::array();
        std::vector<size_t> sorted_indices = sort_indices(*non_accept);
        for (auto i : sorted_indices) {
            json solution_json;// Init
            solution_json["Non-Acceptability"] = (*non_accept)[i];

            json action_map = json::object();

            for (auto state_time_action : policies[i]->policy) {
                int stateID = state_time_action.first;
                int actionID = state_time_action.second;
                int time = mdp->states[stateID]->time;
                stringstream ss;
                ss << time << "," << stateID;
                action_map[ss.str()] = *mdp->getActions(*mdp->states[stateID])->at(actionID)->label;
            }
            solution_json["Action_Map"] = action_map;

            if (mdp->non_moralTheoryIdx != -1) {
                ExpectedUtility* cost = dynamic_cast<ExpectedUtility*>(policies[i]->worth[0].expectations[mdp->non_moralTheoryIdx]);
                solution_json["Expected_Cost"] = cost->value;
            }

            solution_json["Expectation"] = json::object();
            for (int thIdx=0; thIdx < mdp->theories.size(); thIdx++) {
                solution_json["Expectation"][mdp->theories[thIdx]->label] = policies[i]->worth[0].expectations[thIdx]->ToString();
            }

            solutions.push_back(solution_json);
            results["solutions"] = solutions;
        }

        // Add input file to end of json for easy processing!
        std::ifstream inputFile(fileIn); // Don't know how/if to put on heap?
        if (!inputFile.is_open()) {
            throw std::runtime_error("Error loading input file: '" + fileIn + "'");
        }
        json inFile  = json::parse(inputFile);
        results.merge_patch(inFile);

        //
        // Save File
        //
        std::ofstream file(fileOut);
        if (file.is_open()) {
            file << results.dump(4);
            file.close();
            std::cout << "JSON file '" << fileOut <<"' created successfully!" << std::endl;
        } else {
            std::cerr << "Could not open the file '" << fileOut <<"' for output." << std::endl;
        }
    }
};



#endif //OUTPUTTER_HPP
