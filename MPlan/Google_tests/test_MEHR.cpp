//
// Created by Simon Kolker on 23/09/2024.
//
#include "TestBase.hpp"
#include "Solver.hpp"
#include "../Runner.hpp"

class MEHR_Tests : public TestBase {
protected:
    double tolerance = 1e-9;


    vector<size_t> getPolicyIdsByStateAction(Runner& r, int state_idx, vector<string> &actions) {
        auto stateActions = r.mdp->getActions(*r.mdp->states[state_idx]);
        // Map action string to Action Idx.
        vector<size_t> actionToIdx(actions.size(), -1);
        for (int i = 0; i < stateActions->size(); ++i) {
            for (size_t aIdx=0; aIdx<actions.size(); ++aIdx) {
                if (stateActions->at(i)->label == actions[aIdx]) {
                    actionToIdx[aIdx] = i;
                }
            }
        }
        vector<size_t> actionToPolicyIdx(actions.size(), -1);
        for (int i=0; i < r.policies.size(); ++i) {
            for (size_t aIdx=0; aIdx<actionToIdx.size(); ++aIdx) {
                if (r.policies.at(i)->policy[state_idx] == actionToIdx[aIdx]) {
                    actionToPolicyIdx[aIdx] = i;
                }
            }
        }
        return actionToPolicyIdx;
    }
};

TEST_F(MEHR_Tests, SimpleTest) {
    Runner runner = Runner("my_test.json");
    runner.solve();

    // Each policy should have 1 non-acceptability--attacks from both theories
    ASSERT_EQ(runner.non_accept[0], 1);
    ASSERT_EQ(runner.non_accept[1], 1);
}


TEST_F(MEHR_Tests, LibraryTest_EqualRanks) {
    Runner runner = Runner("Library/EqualRanks.json");
    runner.solve();

    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    ASSERT_NEAR(runner.non_accept[piIdx[0]], 1, tolerance);
    ASSERT_NEAR(runner.non_accept[piIdx[1]], 0.7, tolerance);

}
TEST_F(MEHR_Tests, LibraryTest_No_Leaks_Priority) {
    Runner runner = Runner("Library/NoLeaksPriority.json");
    runner.solve();

    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    ASSERT_NEAR(runner.non_accept[piIdx[0]], 1, tolerance);
    ASSERT_NEAR(runner.non_accept[piIdx[1]], 0, tolerance);

}
TEST_F(MEHR_Tests, LibraryTest_Utility_Priority) {
    Runner runner = Runner("Library/UtilityPriority.json");
    runner.solve();
    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    ASSERT_NEAR(runner.non_accept[piIdx[0]], 0, tolerance);
    ASSERT_NEAR(runner.non_accept[piIdx[1]], 0.7, tolerance);

}

TEST_F(MEHR_Tests, CheckForAttack) {
    auto run = Runner("check_for_attack.json");
    run.timePlan();
    run.timeExtractSols();
    run.timeExtractHists();

    vector<string> actions = {"A", "B"};
    auto piIdx = getPolicyIdsByStateAction(run, 0, actions);
    vector<int> theories = {0};

    MEHR mehr = MEHR(*run.mdp, run.histories, run.polExpectations, true);
    auto hRT = vector<vector<vector<int>>>();
    mehr.sortHistories(run.histories, hRT);

    // Actual test
    double r = mehr.checkForAttack((int)piIdx[0], (int)piIdx[1], theories, hRT);
    // Check right non-acceptability
    ASSERT_NEAR(r, 0.75, tolerance);
    // Check only 1 history remaining.
    ASSERT_EQ(hRT[0][piIdx[1]].size(), 1);
    // Check remaining history is the correct one
    auto last_hist = run.histories[piIdx[1]][hRT[0][piIdx[1]][0]];
    ASSERT_EQ(dynamic_cast<ExpectedUtility*>(last_hist->worth.expectations[0])->value, 4);
    ASSERT_EQ(dynamic_cast<ExpectedUtility*>(last_hist->worth.expectations[1])->value, 0);


}
