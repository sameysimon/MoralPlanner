//
// Created by Simon Kolker on 19/09/2024.
//
#include "TestBase.hpp"
#include <Solver.hpp>

using namespace std;

class ExtractOutcomes : public TestBase {

};

TEST_F(ExtractOutcomes, SimpleTest) {
    MDP* mdp = MakeMDP("my_test.json");
    Solver solver = Solver(*mdp);
    vector<shared_ptr<Solution>> solSet = solver.MOValueIteration();

    bool found = false;

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<vector<QValue>*>();
    auto outcomeProbability = vector<vector<double>*>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(solSet, solExpectations, outcomeWorths, outcomeProbability);
    // Check number of solutions
    ASSERT_EQ(solExpectations.size(), 2);
    ASSERT_EQ(outcomeWorths.size(), 2);
    ASSERT_EQ(outcomeProbability.size(), 2);

    //
    // Check solution expectations.
    //
    // Solution 1
    checkExpectedUtility(solExpectations[0], 0, 1, found);
    ASSERT_EQ(found, true);
    checkExpectedUtility(solExpectations[0], 1, 0, found);
    ASSERT_EQ(found, true);
    // Solution 2
    checkExpectedUtility(solExpectations[1], 0, 0, found);
    ASSERT_EQ(found, true);
    checkExpectedUtility(solExpectations[1], 1, 1, found);
    ASSERT_EQ(found, true);

    //
    // Check Outcome Worths
    //
    // Solution 1
    vector<QValue>* worths = outcomeWorths[0];
    vector<double>* probs = outcomeProbability[0];
    if (worths->size()!=1) {
        FAIL() << "1st Solution should have one aggregated outcome QValue.";
    }
    if (probs->size()!=1) {
        FAIL() << "1st solution should have one aggregated outcome probability.";
    }
    checkExpectedUtility(worths->at(0), 0, 1, found);
    ASSERT_EQ(found, true);
    checkExpectedUtility(worths->at(0), 1, 0, found);
    ASSERT_EQ(found, true);
    ASSERT_EQ(probs->at(0), 1);

    // Solution 2
    worths = outcomeWorths[1];
    probs = outcomeProbability[1];
    if (worths->size()!=1) {
        FAIL() << "2nd Solution should have one aggregated outcome QValue.";
    }
    if (probs->size()!=1) {
        FAIL() << "2nd solution should have one aggregated outcome probability.";MDP* mdp = MakeMDP("level_2_prune.json");
    }
    checkExpectedUtility(worths->at(0), 0, 0, found);
    ASSERT_EQ(found, true);
    checkExpectedUtility(worths->at(0), 1, 1, found);
    ASSERT_EQ(found, true);
    ASSERT_EQ(probs->at(0), 1);
}


TEST_F(ExtractOutcomes, Level2Prune) {
    MDP* mdp = MakeMDP("level_2_prune.json");
    Solver solver = Solver(*mdp);
    vector<shared_ptr<Solution>> solSet = solver.MOValueIteration();
    bool found = false;

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<vector<QValue>*>();
    auto outcomeProbability = vector<vector<double>*>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(solSet, solExpectations, outcomeWorths, outcomeProbability);
    // Check number of solutions
    ASSERT_EQ(solExpectations.size(), 2);
    ASSERT_EQ(outcomeWorths.size(), 2);
    ASSERT_EQ(outcomeProbability.size(), 2);
}

TEST_F(ExtractOutcomes, Tree) {
    MDP* mdp = MakeMDP("combos.json");
    Solver solver = Solver(*mdp);
    vector<shared_ptr<Solution>> solSet = solver.MOValueIteration();
    bool found = false;

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<vector<QValue>*>();
    auto outcomeProbability = vector<vector<double>*>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(solSet, solExpectations, outcomeWorths, outcomeProbability);
    // Check number of solutions
    ASSERT_EQ(solExpectations.size(), 8);
    ASSERT_EQ(outcomeWorths.size(), 8);
    ASSERT_EQ(outcomeProbability.size(), 8);

    // All A's solution
    vector<int> actions = {0,0,0};
    for (int i=0; i < solSet.size(); ++i) {
        Solution& sol = *solSet[i].get();
        bool all = true;
        if (checkPolicyStateInTime(sol, 0, actions)) {
            ASSERT_EQ(outcomeWorths[i]->size(), 5);
            ASSERT_EQ(outcomeProbability[i]->size(), 5);
            all = checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 0, 0, 0.5);
            all = all and checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 3, 0, 0.125);
            all = all and checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 1, 2, 0.125);
            all = all and checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 1, 2, 0.125);
            all = all and checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 0, 6, 0.125);
            all = all and checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 5, 1, 0.125);
            ASSERT_TRUE(all);
        }
    }

    // Choose B at the start
    actions = {1,0,0};
    for (int i=0; i < solSet.size(); ++i) {
        Solution& sol = *solSet[i].get();
        if (checkPolicyStateInTime(sol, 0, actions)) {
            ASSERT_EQ(outcomeWorths[i]->size(), 1);
            ASSERT_EQ(outcomeProbability[i]->size(), 1);
            bool all = checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 0, 2, 1);
            ASSERT_TRUE(all);
        }
    }
}