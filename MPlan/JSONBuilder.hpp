
#pragma once
#include <nlohmann/json.hpp>
#include "MEHR.hpp"
#include "Solver.hpp"

using json = nlohmann::json;
struct FIELD {
    static constexpr const char* EXPANDED = "Expanded";
    static constexpr const char* BACKUPS = "Backups";
    static constexpr const char* ITERATIONS = "Iterations";
    static constexpr const char* SOLUTION_TOTAL = "SolutionTotal";
    static constexpr const char* AVG_HISTORIES = "Average_histories";
    static constexpr const char* MIN_HISTORIES = "Min_histories";
    static constexpr const char* MAX_HISTORIES = "Max_histories";
    static constexpr const char* DURATION_HEURISTIC = "Duration_Heuristic";
    static constexpr const char* DURATION_PLAN = "Duration_Plan";
    static constexpr const char* DURATION_SOLS = "Duration_Sols";
    static constexpr const char* DURATION_SOLS_REDUCE = "Duration_Sols_Reduce";
    static constexpr const char* DURATION_OUTS = "Duration_Outs";
    static constexpr const char* DURATION_MEHR = "Duration_MEHR";
    static constexpr const char* DURATION_CQ_1 = "Duration_CQ1";
    static constexpr const char* DURATION_CQ_2 = "Duration_CQ2";
    static constexpr const char* DURATION_TOTAL = "Duration_Total";
    static constexpr const char* SOLUTIONS = "Solutions";
    static constexpr const char* NON_ACCEPTABILITY = "Acceptability";
    static constexpr const char* NUM_OF_MIN_NON_ACCEPT = "Num_Min_Non_Acceptability";
    static constexpr const char* ACTION_MAP = "Action_Map";
    static constexpr const char* EXPECTED_COST = "Expected_Cost";
    static constexpr const char* EXPECTATION = "Expectation";
    static constexpr const char* DATA = "Data";
    static constexpr const char* WORTH = "Worth";
    static constexpr const char* PATH = "Path";
    static constexpr const char* PROBABILITY = "Probability";
    static constexpr const char* HISTORIES = "Histories";
    static constexpr const char* FOILSOLUTIONS = "FoilSolutions";
    static constexpr const char* ATTACKS = "Attacks";

    static constexpr const char* SOURCEPOLICY = "SourcePolicyIdx";
    static constexpr const char* SOURCEHISTORY = "SourceHistoryIdx";
    static constexpr const char* TARGETPOLICY = "TargetPolicyIdx";
    static constexpr const char* TARGETHISTORY = "TargetHistoryIdx";
    static constexpr const char* THEORY = "Theory";

    static constexpr const char* THEORIES = "Theories";
    static constexpr const char* CONSIDERATIONS = "Considerations";

    static constexpr const char* SOLUTIONS_ORDER = "Solutions_Order";
};
struct explainResult;
struct Durations;
class Solver;
class Runner;
class JSONBuilder {
public:
    static json toJSON(Runner& run);
    static json toJSON(const std::vector<Attack>& attackVector);
    static json toJSON(explainResult &er, Runner& runner);

    static json toJSON(vector<unique_ptr<Policy>>& policies, MDP &mdp, NonAcceptability &non_accept, bool includeActions=true);
    static json toJSON(Policy &pi, MDP &mdp, double non_accept, bool includeActions=true);

    static json toJSON(vector<vector<History*>>& histories, bool includePaths=false);
    // JSON {probability: double, worth: string, path: [[sourceStateID, targetStateID], ...]}
    static json toJSON(History& h);
    static json toJSON(vector<Successor*>& path);
    static json toJSON(Durations& dur);
    static json toJSON(vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction, MDP& mdp, State& s);
    static json toJSON(Solver& solver);
private:
    static void buildPolicyMap(json &action_map, Policy& pi, MDP& mdp) {
      for (auto map : pi.policy) {
          int stateID = map.first;
          int actionID = pi.getAction(map.first);
          if (actionID==-1) {
              continue;
          }
          action_map[std::to_string(stateID)] = mdp.getActions(*mdp.states[stateID])->at(actionID)->label;
      }
    }
    std::vector<size_t> static sort_indices(NonAcceptability& non_accept) {
        // Create a vector of indices (0, 1, 2, ..., non_accept.size() - 1)
        std::vector<size_t> indices(non_accept.getTotalPolicies());
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }
        // Sort indices based on comparing the values in non_accept
        std::sort(indices.begin(), indices.end(),
                  [&non_accept](size_t i1, size_t i2) { return non_accept.getPolicyNonAccept(i1) < non_accept.getPolicyNonAccept(i2); });

        return indices;
    }


};
