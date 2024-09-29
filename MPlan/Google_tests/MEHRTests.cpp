//
// Created by Simon Kolker on 23/09/2024.
//
#include "TestBase.hpp"
#include "Solver.hpp"

class MEHR_Tests : public TestBase {

};


TEST_F(MEHR_Tests, SimpleTest) {
    MDP* mdp = MakeMDP("my_test.json");
    Solver solver = Solver(*mdp);
    vector<shared_ptr<Solution>> solSet = solver.MOValueIteration();

    bool found = false;

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<vector<QValue>*>();
    auto outcomeProbability = vector<vector<double>*>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(solSet, solExpectations, outcomeWorths, outcomeProbability);
    vector<double>* nonAccept = mehr.findNonAccept(solExpectations, outcomeWorths, outcomeProbability);
    // Each policy should have 1 non-acceptability--attacks from both theories
    ASSERT_EQ((*nonAccept)[0], 1);
    ASSERT_EQ((*nonAccept)[1], 1);

}

