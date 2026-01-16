//
// Created by Simon Kolker on 19/09/2024.
//
#include "TestBase.hpp"
#include <Solver.hpp>

using namespace std;

class ExtractHistoriesTest : public TestBase {
protected:
    vector<vector<History*>> histories;
    void loadHistoriesFrom(const std::string& fileName) {
        // Make MDP
        MDP* mdp = getMDP(fileName);
        // Solve MDP
        Solver solver = Solver(*mdp);
        solver.MC_iAO_Star();
        // Extract Policies.
        vector<unique_ptr<Policy>> policies;
        solver.getSolutions(policies);
        // Extract Histories
        auto eh = new ExtractHistories(*mdp);
        eh->extract(histories, policies);
        delete eh;
        delete mdp;
    }


    static bool compareHistories(const History& h1, const History& h2) {
        return h1.probability == h2.probability && h1.worth==h2.worth;
    }
    static bool searchHistories(vector<History*>& piHistories, History& h) {
        return std::any_of(piHistories.begin(), piHistories.end(),
            [&h](History* h_) { return compareHistories(h, *h_); }
        );
    }
    static bool findPolicyWithExpectedHistories(std::vector<int> &policies, vector<vector<History*>> &histories, vector<History> &expectedHistories) {
        bool found;
        for (auto it = policies.begin(); it != policies.end();) {
            found = true;
            for (auto &h : expectedHistories) {
                if (!searchHistories(histories[*it.base()], h)) {
                    found = false; // This policy is not good.
                    break;
                }
            }
            if (found) {
                // This policy has all the histories we need!
                it = policies.erase(it);
            } else {
                it++;
            }
        }
        return true;
    }
};

TEST_F(ExtractHistoriesTest, SimpleTest) {
    // Loads histories into class's histories vector.
    loadHistoriesFrom("my_test.json");
    std::vector<int> unfoundPolicies(histories.size());
    std::iota(unfoundPolicies.begin(), unfoundPolicies.end(), 0);
    vector<History> expectedHistories;

    //
    // Check History for Policy 1
    //
    expectedHistories = {History(BuildUtilityQValue({1,0}), 1.0)};
    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));

    //
    // Check History for Policy 2
    //
    expectedHistories = {History(BuildUtilityQValue({0,1}), 1.0)};
    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));
}

TEST_F(ExtractHistoriesTest, Level2Prune) {
    loadHistoriesFrom("level_2_prune.json");
    std::vector<int> unfoundPolicies(histories.size());
    std::iota(unfoundPolicies.begin(), unfoundPolicies.end(), 0);
    vector<History> expectedHistories;
    //
    // Check History for Policy 1
    //
    expectedHistories = {History(BuildUtilityQValue({3,0}), 1.0)};
    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));

}

TEST_F(ExtractHistoriesTest, Tree) {
    loadHistoriesFrom("combos.json");
    std::vector<int> unfoundPolicies(histories.size());
    std::iota(unfoundPolicies.begin(), unfoundPolicies.end(), 0);
    vector<History> expectedHistories;

    //
    // Check History for Policy 1
    //
    ASSERT_EQ(histories[0].size(), 5);
    expectedHistories = {
        History(BuildUtilityQValue({0,0}), 0.5),
        History(BuildUtilityQValue({3,0}), 0.125),
        History(BuildUtilityQValue({1,2}), 0.125),
        History(BuildUtilityQValue({0,6}), 0.125),
        History(BuildUtilityQValue({5,0}), 0.125),
    };

    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));

}