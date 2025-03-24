//
// Created by Simon Kolker on 03/10/2024.
//

#pragma once
#include "MDP.hpp"
#include <nlohmann/json.hpp>
#include "Solver.hpp"
#include "Utilitarianism.hpp"
#include "ExtractHistories.hpp"
#include <fstream>
#include <sstream>
#include "Runner.hpp"
#include <utility>

using json = nlohmann::json;
using namespace std;

struct FIELD {
    static constexpr const char* EXPANDED = "Expanded";
    static constexpr const char* BACKUPS = "Backups";
    static constexpr const char* ITERATIONS = "Iterations";
    static constexpr const char* SOLUTION_TOTAL = "SolutionTotal";
    static constexpr const char* AVG_HISTORIES = "average_histories";
    static constexpr const char* MIN_HISTORIES = "min_histories";
    static constexpr const char* MAX_HISTORIES = "max_histories";
    static constexpr const char* DURATION_PLAN = "duration_Plan";
    static constexpr const char* DURATION_SOLS = "duration_Sols";
    static constexpr const char* DURATION_OUTS = "duration_Outs";
    static constexpr const char* DURATION_MEHR = "duration_MEHR";
    static constexpr const char* DURATION_TOTAL = "duration_Total";
    static constexpr const char* SOLUTIONS = "solutions";
    static constexpr const char* NON_ACCEPTABILITY = "Non-Acceptability";
    static constexpr const char* ACTION_MAP = "Action_Map";
    static constexpr const char* EXPECTED_COST = "Expected_Cost";
    static constexpr const char* EXPECTATION = "Expectation";
};


class Output {
    MDP* mdp;
    json results;
    void buildPolicyMap(json &action_map, Policy* pi) {
        for (auto state_time_action : pi->policy) {
            int stateID = state_time_action.first;
            int actionID = state_time_action.second;
            int time = mdp->states[stateID]->time;
            stringstream ss;
            ss << time << "," << stateID;
            action_map[ss.str()] = mdp->getActions(*mdp->states[stateID])->at(actionID)->label;
        }
    }
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
public:
    Output(MDP* mdp, Solver &solver) : mdp(mdp) {
        results[FIELD::EXPANDED] = solver.expanded_states;
        results[FIELD::BACKUPS] = solver.backups;
        results[FIELD::ITERATIONS] = solver.iterations;
    }

    void addDuration(long long plan, long long sols, long long outs, long long mehr) {
        results[FIELD::DURATION_PLAN] = plan;
        results[FIELD::DURATION_SOLS] = sols;
        results[FIELD::DURATION_OUTS] = outs;
        results[FIELD::DURATION_MEHR] = mehr;
        results[FIELD::DURATION_TOTAL] = plan + sols + outs + mehr;
    }

    void addHistories(vector<vector<History*>>& histories) {
        size_t max=0;
        size_t min=histories[0].size();
        unsigned long avg=0;
        for (const auto& hist : histories) {
            avg+=hist.size();
            max = std::max(max, hist.size());
            min = std::min(min, hist.size());
        }
        avg = avg / (unsigned long)histories.size();
        results[FIELD::AVG_HISTORIES] = avg;
        results[FIELD::MIN_HISTORIES] = min;
        results[FIELD::MAX_HISTORIES] = max;
    }

    void addPolicies(vector<Policy*>& policies, vector<double> &non_accept) {
        json solutions = json::array();
        // Add solutions in order of non-acceptability. First solution has lowest Non Accept.
        std::vector<size_t> sorted_indices = sort_indices(non_accept);
        for (auto i : sorted_indices) {
            json solution_json;// Init
            // Set non-acceptability
            solution_json[FIELD::NON_ACCEPTABILITY] = (non_accept)[i];

            // Build action map
            json action_map = json::object();
            buildPolicyMap(action_map, policies[i]);
            solution_json[FIELD::ACTION_MAP] = action_map;

            // Add Expectd Cost
            if (mdp->non_moralTheoryIdx != -1) {
                ExpectedUtility* cost = dynamic_cast<ExpectedUtility*>(policies[i]->worth[0].expectations[mdp->non_moralTheoryIdx]);
                solution_json[FIELD::EXPECTED_COST] = cost->value;
            }

            // Add policy's expectations for all moral theories.
            solution_json[FIELD::EXPECTATION] = json::object();
            for (int thIdx=0; thIdx < mdp->theories.size(); thIdx++) {
                solution_json[FIELD::EXPECTATION][mdp->theories[thIdx]->label] = policies[i]->worth[0].expectations[thIdx]->ToString();
            }

            solutions.push_back(solution_json);
            results[FIELD::SOLUTIONS] = solutions;
        }
    }


    void writeToFile(const std::string& fileOut, const std::string& fileIn) {
        // Add input file to end of json for easy processing!
        std::ifstream inputFile(fileIn); // Don't know how/if to put on heap?
        if (!inputFile.is_open()) {
            throw std::runtime_error("Error loading input file: '" + fileIn + "'");
        }
        json inFile  = json::parse(inputFile);
        results.merge_patch(inFile);

        // Save File
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



