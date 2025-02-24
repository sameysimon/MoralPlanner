//
// Created by Simon Kolker on 23/09/2024.
//
#include "TestBase.hpp"
#include "Solver.hpp"

class MEHR_Tests : public TestBase {
protected:
    double tolerance = 1e-9;
};
/*
TEST_F(MEHR_Tests, SimpleTest) {
    MDP* mdp = MakeMDP("my_test.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);
    vector<double>* nonAccept = mehr.findNonAccept(solExpectations, outcomeWorths, outcomeProbability);
    // Each policy should have 1 non-acceptability--attacks from both theories
    ASSERT_EQ((*nonAccept)[0], 1);
    ASSERT_EQ((*nonAccept)[1], 1);

}

TEST_F(MEHR_Tests, LibraryTest_EqualRanks) {
    MDP* mdp = MakeMDP("Library/EqualRanks.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);

    vector<double>* nonAccept = mehr.findNonAccept(solExpectations, outcomeWorths, outcomeProbability);

    int ignoreStateAction=0;
    int recommendStateAction=0;
    auto stateActions = mdp->getActions(*mdp->states[0], 0);
    for (int i = 0; i < stateActions->size(); ++i) {
        if (*stateActions->at(i)->label==std::string("Recommend")) {
            recommendStateAction = i;
        }
        if (*stateActions->at(i)->label==std::string("Ignore")) {
            recommendStateAction = i;
        }
    }

    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == recommendStateAction) {
            ASSERT_NEAR((*nonAccept)[i], 1, tolerance);
            break;
        }
    }
    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == ignoreStateAction) {
            ASSERT_NEAR((*nonAccept)[i], 0.7, tolerance);
            break;
        }
    }
}
TEST_F(MEHR_Tests, LibraryTest_No_Leaks_Priority) {
    MDP* mdp = MakeMDP("Library/NoLeaksPriority.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);

    vector<double>* nonAccept = mehr.findNonAccept(solExpectations, outcomeWorths, outcomeProbability);

    std::cout << nonAccept->at(0) << std::endl;
    std::cout << nonAccept->at(1) << std::endl;
    int ignoreStateAction=0;
    int recommendStateAction=0;
    auto stateActions = mdp->getActions(*mdp->states[0], 0);
    for (int i = 0; i < stateActions->size(); ++i) {
        if (*stateActions->at(i)->label==std::string("Recommend")) {
            recommendStateAction = i;
        }
        if (*stateActions->at(i)->label==std::string("Ignore")) {
            recommendStateAction = i;
        }
    }

    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == recommendStateAction) {
            ASSERT_NEAR((*nonAccept)[i], 1, tolerance);
            break;
        }
    }
    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == ignoreStateAction) {
            ASSERT_NEAR((*nonAccept)[i], 0, tolerance);
            break;
        }
    }
}
TEST_F(MEHR_Tests, LibraryTest_Utility_Priority) {
    MDP* mdp = MakeMDP("Library/UtilityPriority.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);

    vector<double>* nonAccept = mehr.findNonAccept(solExpectations, outcomeWorths, outcomeProbability);

    int ignoreStateAction=0;
    int recommendStateAction=0;
    auto stateActions = mdp->getActions(*mdp->states[0]);
    for (int i = 0; i < stateActions->size(); ++i) {
        if (*stateActions->at(i)->label==std::string("Recommend")) {
            recommendStateAction = i;
        }
        if (*stateActions->at(i)->label==std::string("Ignore")) {
            recommendStateAction = i;
        }
    }

    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == recommendStateAction) {
            ASSERT_NEAR((*nonAccept)[i], 0, tolerance);
            break;
        }
    }
    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == ignoreStateAction) {
            ASSERT_NEAR((*nonAccept)[i], 0.7, tolerance);
            break;
        }
    }
}*/
