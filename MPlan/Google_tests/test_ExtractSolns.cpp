

#include <iostream>
#include "TestBase.hpp"
#include <Solver.hpp>

using namespace std;
class ExtractSolutionsTest : public TestBase {
protected:
    vector<unique_ptr<Policy>> policies;
    vector<vector<unique_ptr<History>>> histories;
    void loadPolicyAndHistoriesFrom(const std::string& fileName) {
        // Make MDP
        MDP mdp = MDP(fileName);
        // Solve MDP
        Solver solver = Solver(mdp);
        solver.MC_iAO_Star();
        // Extract Policies.
        auto soln_extractor = SolutionExtracter(mdp);
        soln_extractor.Extract(policies, histories, solver.mPi);
    }
    vector<size_t> findPoliciesWithUtility(list<size_t>& policy_indices, vector<double>& util_vector) {
        vector<size_t> matches;
        for (const auto& pi_idx : policy_indices) {
            auto &policy = policies[pi_idx];
            auto worth = policy->getExpectationPtr();
            bool isMatch = true;
            for (size_t i = 0; i < util_vector.size(); ++i) {
                auto &c = worth->expectations[i];
                auto* u = dynamic_cast<ExpectedUtility*>(c.get());
                if (abs(u->value - util_vector[i]) > 1e-6 ) {
                    isMatch = false;
                    break;
                }
            }
            if (isMatch) {
                matches.push_back(pi_idx);
            }
        }
        return matches;
    }
    static bool compareHistories(const History& h1, const History& h2) {
        return h1.probability == h2.probability && h1.mWorth==h2.mWorth;
    }
    static bool searchHistories(vector<unique_ptr<History>>& piHistories, History& h) {
        return std::any_of(piHistories.begin(), piHistories.end(),
            [&h](unique_ptr<History>& h_) { return compareHistories(h, *h_); }
        );
    }
    static optional<size_t> findPolicyWithExpectedHistories(std::vector<size_t> &policies, policy_hists &histories, vector<History> &expectedHistories) {
        bool found;
        for (auto it = policies.begin(); it != policies.end(); ++it) {
            found = true;
            for (auto &h : expectedHistories) {
                if (!searchHistories(histories[*it.base()], h)) {
                    found = false; // This policy is not good.
                    break;
                }
            }
            if (found) {
                return *it.base();
            }
        }
        return nullopt;
    }
    void test_against_json_oracle(const string& fileName) {
        std::ifstream file(fileName);
        json data = json::parse(file);
        loadPolicyAndHistoriesFrom(fileName);
        data = data["Test_oracle"];
        size_t expected_set_size = data["Expected_coverage_size"];
        ASSERT_GE(policies.size(), expected_set_size);

        list<size_t> remaining_policies(policies.size(),0);
        iota(remaining_policies.begin(), remaining_policies.end(), 0);

        for (auto &exp_policy_data : data["Expected_coverage"]) {
            vector<double> expWorth = exp_policy_data["Expectation"];

            auto candidateIndices = findPoliciesWithUtility(remaining_policies, expWorth);
            ASSERT_GT(candidateIndices.size(), 0);
            vector<History> expHistories;
            for (auto &traj_data : exp_policy_data["Distribution"]) {
                vector<double> trajWorth = traj_data["Worth"];
                expHistories.emplace_back(BuildUtilityQValue(trajWorth), traj_data["Probability"]);
                auto optPi = findPolicyWithExpectedHistories(candidateIndices, histories, expHistories);
                ASSERT_TRUE(optPi.has_value());
                remaining_policies.remove(optPi.value());
            }
        }
    }

};

TEST_F(ExtractSolutionsTest, NewTest) {
    test_against_json_oracle("/Users/user/Desktop/MyMoralPlanner/MoralPlanner/simple_two_objective_front.json");
    test_against_json_oracle("/Users/user/Desktop/MyMoralPlanner/MoralPlanner/two_level_tree_combinations.json");
    test_against_json_oracle("/Users/user/Desktop/MyMoralPlanner/MoralPlanner/semantic_duplicate_actions.json");
    test_against_json_oracle("/Users/user/Desktop/MyMoralPlanner/MoralPlanner/reconvergent_action_compatibility.json");
    test_against_json_oracle("/Users/user/Desktop/MyMoralPlanner/MoralPlanner/Data/MDPs/reconvergent_witness_required.json");
}
