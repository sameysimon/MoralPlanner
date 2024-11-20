//
// Created by Simon Kolker on 19/09/2024.
//
#include "TestBase.hpp"
#include <Solver.hpp>

using namespace std;

class ExtractHistories : public TestBase {

};

TEST_F(ExtractHistories, SimpleTest) {
    MDP* mdp = MakeMDP("my_test.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    vector<shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    bool found = false;

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);
    // Check number of solutions
    ASSERT_EQ(solExpectations.size(), 2);
    ASSERT_EQ(outcomeWorths.size(), 2);
    ASSERT_EQ(outcomeProbability.size(), 2);

    //
    // Check solution expectations.
    //
    // Solution 1
    bool foundSol_1_Expectation = false;
    bool found_Sol_1_Out_1_Theory_1 = false;
    bool found_Sol_1_Out_1_Theory_2 = false;
    bool found_Sol_1_Prob_1 = false;
    for (int i=0; i < solSet->size(); ++i) {
        auto sol = solSet->at(i);
        if (checkPolicyAtTime(sol, 0, 0, 0)) {
            checkExpectedUtility(solExpectations[i], 0, 1, foundSol_1_Expectation);
            for (int outIdx = 0; outIdx < outcomeWorths[i]->size(); ++outIdx) {
                checkExpectedUtility(outcomeWorths[i]->at(outIdx), 0, 1, found_Sol_1_Out_1_Theory_1);
                checkExpectedUtility(outcomeWorths[i]->at(outIdx), 1, 0, found_Sol_1_Out_1_Theory_2);
                found_Sol_1_Prob_1 = outcomeProbability[i]->at(outIdx)== 1 or found_Sol_1_Prob_1;

            }
        }
    }
    ASSERT_TRUE(foundSol_1_Expectation);
    ASSERT_TRUE(found_Sol_1_Out_1_Theory_1);
    ASSERT_TRUE(found_Sol_1_Out_1_Theory_2);
    ASSERT_TRUE(found_Sol_1_Prob_1);

    bool foundSol_2_Expectation = false;
    bool found_Sol_2_Out_1_Theory_1 = false;
    bool found_Sol_2_Out_1_Theory_2 = false;
    bool found_Sol_2_Prob_1 = false;
    for (int i=0; i < solSet->size(); ++i) {
        auto sol = solSet->at(i);
        if (checkPolicyAtTime(sol, 0, 0, 1)) {
            checkExpectedUtility(solExpectations[i], 0, 0, foundSol_2_Expectation);
            checkExpectedUtility(solExpectations[i], 1, 1, foundSol_2_Expectation);
            for (int outIdx = 0; outIdx < outcomeWorths[i]->size(); ++outIdx) {
                checkExpectedUtility(outcomeWorths[i]->at(outIdx), 0, 0, found_Sol_2_Out_1_Theory_1);
                checkExpectedUtility(outcomeWorths[i]->at(outIdx), 1, 1, found_Sol_2_Out_1_Theory_2);
                found_Sol_2_Prob_1 = outcomeProbability[i]->at(outIdx)== 1 or found_Sol_2_Prob_1;

            }
        }
    }
    ASSERT_TRUE(foundSol_2_Expectation);
    ASSERT_TRUE(found_Sol_2_Out_1_Theory_1);
    ASSERT_TRUE(found_Sol_2_Out_1_Theory_2);
    ASSERT_TRUE(found_Sol_2_Prob_1);

}


TEST_F(ExtractHistories, Level2Prune) {
    MDP* mdp = MakeMDP("level_2_prune.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    vector<shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);
    // Check number of solutions
    ASSERT_EQ(solExpectations.size(), 1);
    ASSERT_EQ(outcomeWorths.size(), 1);
    ASSERT_EQ(outcomeProbability.size(), 1);

    ASSERT_TRUE(checkContainsQValue(outcomeWorths[0], outcomeProbability[0], 3, 0, 1));
}


TEST_F(ExtractHistories, Tree) {
    MDP* mdp = MakeMDP("combos.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    vector<shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);


    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);
    // Check number of solutions
    ASSERT_EQ(solExpectations.size(), 2);
    ASSERT_EQ(outcomeWorths.size(), 2);
    ASSERT_EQ(outcomeProbability.size(), 2);

    // All A's solution
    for (int i=0; i < solSet->size(); ++i) {
        auto sol = solSet->at(i);
        if (checkPolicyAtTime(sol, 0, 0, 0) and checkPolicyAtTime(sol, 1, 1, 0) and checkPolicyAtTime(sol, 4, 2, 0) and checkPolicyAtTime(sol, 5, 2, 0)) {
            ASSERT_EQ(outcomeWorths[i]->size(), 5);
            ASSERT_EQ(outcomeProbability[i]->size(), 5);
            ASSERT_TRUE(checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 0, 0, 0.5));
            ASSERT_TRUE(checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 3, 0, 0.125));
            ASSERT_TRUE(checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 1, 2, 0.125));
            ASSERT_TRUE(checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 1, 2, 0.125));
            ASSERT_TRUE(checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 0, 6, 0.125));
            ASSERT_TRUE(checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 5, 1, 0.125));
        }
    }

    // Choose B at the start
    for (int i=0; i < solSet->size(); ++i) {
        auto sol = solSet->at(i);
        if (checkPolicyAtTime(sol, 0, 0, 1) and checkPolicyAtTime(sol, 3, 1, 0) and checkPolicyAtTime(sol, 3, 1, 0) and checkPolicyAtTime(sol, 3, 1, 0)) {
            ASSERT_EQ(outcomeWorths[i]->size(), 1);
            ASSERT_EQ(outcomeProbability[i]->size(), 1);
            ASSERT_TRUE(checkContainsQValue(outcomeWorths[i], outcomeProbability[i], 0, 3, 1));
        }
    }
}