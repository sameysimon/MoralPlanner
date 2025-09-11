//
// Created by Simon Kolker on 23/09/2024.
//
#include "TestBase.hpp"
#include "Logger.hpp"


class MEHR_Tests : public TestBase {
protected:
    double tolerance = 1e-9;
    // Assumes Utilitarianism for now
    static void AssertOrder(Runner &run, size_t policyIdx, size_t theoryIdx) {
        auto mehrUtil = dynamic_cast<MEHRUtilitarianism*>(run.mdp->mehr_theories[theoryIdx]);
        auto orderedHistories = mehrUtil->getSortedHistories()->orderedHistories;
        auto policyHistories = run.histories[policyIdx];
        // Histories should be sorted into descending order.
        size_t largestHistoryIdx = orderedHistories[policyIdx][0];
        auto lastUtil = dynamic_cast<ExpectedUtility*>(policyHistories[largestHistoryIdx]->worth.expectations[theoryIdx])->value;
        for (auto hIdx : orderedHistories[policyIdx]) {
            auto currUtil = dynamic_cast<ExpectedUtility*>(policyHistories[hIdx]->worth.expectations[theoryIdx])->value;
            ASSERT_LE(currUtil, lastUtil) << "Histories out of order. Theory '" << theoryIdx <<"'. History Idx:" << hIdx << ", utility:" << currUtil << " is greater than previous History Idx:" << hIdx-1 << ", utility:" << lastUtil;
            lastUtil = currUtil;
        }
    }
};

TEST_F(MEHR_Tests, SimpleTest) {
    Runner runner = Runner("my_test.json");
    runner.solve();

    // Each policy should have 1 non-acceptability--attacks from both theories
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(0), 1);
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(1), 1);
}


TEST_F(MEHR_Tests, LibraryTest_EqualRanks) {
    Runner runner = Runner("Library/EqualRanks.json");
    runner.solve();

    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    ASSERT_NEAR(runner.non_accept.getPolicyNonAccept(piIdx[0]), 1, tolerance);
    ASSERT_NEAR(runner.non_accept.getPolicyNonAccept(piIdx[1]), 0.7, tolerance);

}
TEST_F(MEHR_Tests, LibraryTest_No_Leaks_Priority) {
    Runner runner = Runner("Library/NoLeaksPriority.json");
    runner.solve();

    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    ASSERT_NEAR(runner.non_accept.getPolicyNonAccept(piIdx[0]), 1, tolerance);
    ASSERT_NEAR(runner.non_accept.getPolicyNonAccept(piIdx[1]), 0, tolerance);

}
TEST_F(MEHR_Tests, LibraryTest_Utility_Priority) {
    Runner runner = Runner("Library/UtilityPriority.json");
    runner.solve();
    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    ASSERT_NEAR(runner.non_accept.getPolicyNonAccept(piIdx[0]), 0, tolerance);
    ASSERT_NEAR(runner.non_accept.getPolicyNonAccept(piIdx[1]), 0.7, tolerance);

}

TEST_F(MEHR_Tests, SortHistories) {
    auto run = Runner("check_for_attack.json");
    run.timePlan();
    run.timeExtractSols();
    run.timeExtractHists();

    vector<string> actions = {"A", "B"};
    auto piIdx = getPolicyIdsByStateAction(run, 0, actions);

    MEHR mehr = MEHR(*run.mdp, run.policies, run.histories);


    // Get policy-histories for theory 0
    AssertOrder(run, piIdx[0], 0);
    AssertOrder(run, piIdx[1], 0);
    AssertOrder(run, piIdx[1], 1);
    AssertOrder(run, piIdx[1], 1);
}


TEST_F(MEHR_Tests, CheckForAttack) {
    auto run = Runner("check_for_attack.json");
    run.timePlan();
    run.timeExtractSols();
    run.timeExtractHists();

    vector<string> actions = {"A", "B"};
    auto piIdx = getPolicyIdsByStateAction(run, 0, actions);
    vector<int> theories = {0};

    MEHR mehr = MEHR(*run.mdp, run.policies, run.histories);


    // Actual test
    FAIL() << "Refactored CheckForAttack implementation. Must update.";
    //double result = mehr.checkForAttack((int)piIdx[0], (int)piIdx[1], theories);
    // Check right non-acceptability
    //ASSERT_NEAR(result, 0.75, tolerance);


}
