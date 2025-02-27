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
        MDP* mdp = MakeMDP(fileName);

        // Solve MDP
        Solver solver = Solver(*mdp);
        solver.MOiLAO();
        FAIL() << "Fail 1";
        // Extract Policies.
        auto policies = solver.getSolutions();
        FAIL() << "Fail 2";
        // Extract Histories
        auto eh = new ExtractHistories(*mdp);
        FAIL() << "Fail 3";
        histories = eh->extract(*policies);
        FAIL() << "Fail 4";
        delete eh;
        delete policies;
        delete mdp;
        FAIL() << "Fail 5";
    }

    static History* createHistory(vector<WorthBase*>& worth, double prob) {
        auto qv = new QValue();
        qv->expectations.insert(qv->expectations.end(), std::make_move_iterator(worth.begin()),
                                      std::make_move_iterator(worth.end()));
        return new History(*qv, prob);
    }
    static bool compareHistories(const History& h1, const History& h2) {
        return h1.probability == h2.probability && h1.worth==h2.worth;
    }
    static bool searchHistories(vector<History*>& piHistories, History& h) {
        return std::any_of(piHistories.begin(), piHistories.end(),
            [&h](History* h_) { return compareHistories(h, *h_); }
        );
    }
    static bool findPolicyWithExpectedHistories(std::vector<int> &policies, vector<vector<History*>> &histories, vector<History*> &expectedHistories) {
        bool found;
        for (auto it = policies.begin(); it != policies.end();) {
            found = true;
            for (auto &h : expectedHistories) {
                if (!searchHistories(histories[*it.base()], *h)) {
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
    loadHistoriesFrom("my_test.json");
    std::vector<int> unfoundPolicies(histories.size());
    std::iota(unfoundPolicies.begin(), unfoundPolicies.end(), 0);

    //
    // Check History for Policy 1
    //
    vector<WorthBase*> expectedWorths;
    vector<History*> expectedHistories;
    expectedWorths = {new ExpectedUtility(1), new ExpectedUtility(0)};
    expectedHistories.push_back(createHistory(expectedWorths, 1.0));
    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));
    for (auto wb : expectedWorths) { delete wb; }
    for (auto h : expectedHistories) { delete h; }
    expectedHistories.clear();
    expectedWorths.clear();

    //
    // Check History for Policy 2
    //
    expectedWorths = {new ExpectedUtility(0), new ExpectedUtility(1)};
    expectedHistories.push_back(createHistory(expectedWorths, 1.0));
    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));
    for (auto wb : expectedWorths) { delete wb; }
    for (auto h : expectedHistories) { delete h; }
    std::cout << "Test 4." << std::endl;

    //
    // Cleanup actual histories.
    //
    for (auto H : histories) {
        for (auto h : H) {
            delete h;
        }
    }
}

TEST_F(ExtractHistoriesTest, Level2Prune) {
    loadHistoriesFrom("level_2_prune.json");
    std::vector<int> unfoundPolicies(histories.size());
    std::iota(unfoundPolicies.begin(), unfoundPolicies.end(), 0);

    //
    // Check History for Policy 1
    //
    vector<WorthBase*> expectedWorths;
    vector<History*> expectedHistories;
    expectedWorths = {new ExpectedUtility(3), new ExpectedUtility(0)};
    expectedHistories.push_back(createHistory(expectedWorths, 1.0));
    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));
    for (auto wb : expectedWorths) { delete wb; }

    // Cleanup histories
    for (auto H : histories) {
        for (auto h : H) {
            delete h;
        }
    }
    for (auto h : expectedHistories) { delete h; }

}

TEST_F(ExtractHistoriesTest, Tree) {
    loadHistoriesFrom("combos.json");
    std::vector<int> unfoundPolicies(histories.size());
    std::iota(unfoundPolicies.begin(), unfoundPolicies.end(), 0);
    //
    // Check History for Policy 1
    //
    //ASSERT_EQ(histories.size(), 2);
    ASSERT_EQ(histories[0].size(), 5);
    vector<WorthBase*> expectedWorths;
    vector<History*> expectedHistories;
    expectedWorths = {new ExpectedUtility(0), new ExpectedUtility(0)};
    expectedHistories.push_back(createHistory(expectedWorths, 0.5));
    expectedWorths = {new ExpectedUtility(3), new ExpectedUtility(0)};
    expectedHistories.push_back(createHistory(expectedWorths, 0.125));
    expectedWorths = {new ExpectedUtility(1), new ExpectedUtility(2)};
    expectedHistories.push_back(createHistory(expectedWorths, 0.125));
    expectedWorths = {new ExpectedUtility(0), new ExpectedUtility(6)};
    expectedHistories.push_back(createHistory(expectedWorths, 0.125));
    expectedWorths = {new ExpectedUtility(5), new ExpectedUtility(1)};
    expectedHistories.push_back(createHistory(expectedWorths, 0.125));
    ASSERT_TRUE(findPolicyWithExpectedHistories(unfoundPolicies, histories, expectedHistories));
    for (auto wb : expectedWorths) { delete wb; }
    for (auto h : expectedHistories) { delete h; }

    //
    // Cleanup histories
    //
    for (auto H : histories) {
        for (auto h : H) {
            delete h;
        }
    }
}