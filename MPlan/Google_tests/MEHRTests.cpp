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
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double long>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);
    vector<double long>* nonAccept = mehr.findNonAccept(solExpectations, outcomeWorths, outcomeProbability);
    // Each policy should have 1 non-acceptability--attacks from both theories
    ASSERT_EQ((*nonAccept)[0], 1);
    ASSERT_EQ((*nonAccept)[1], 1);

}

TEST_F(MEHR_Tests, LibraryTest_EqualRanks) {
    MDP* mdp = MakeMDP("MEHR_test_1.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);

    auto solExpectations = vector<QValue>();
    auto outcomeWorths = vector<shared_ptr<vector<QValue>>>();
    auto outcomeProbability = vector<shared_ptr<vector<double long>>>();
    MEHR mehr = MEHR(*mdp);
    mehr.extractInfo(*solSet, solExpectations, outcomeWorths, outcomeProbability);

    for (int solIdx=0; solIdx < solSet->size(); solIdx++) {
        double long total = 0;
        std::cout << "Solution " << solIdx << " expects " << solExpectations[solIdx].toString() << std::endl;
        for (int outIdx=0; outIdx < outcomeWorths[solIdx]->size(); outIdx++) {
            std::cout << "      Outcome #" << outIdx << " has " << outcomeWorths[solIdx]->at(outIdx).toString() << " at Probability " <<  outcomeProbability[solIdx]->at(outIdx) << std::endl;
            total += outcomeProbability[solIdx]->at(outIdx);
        }
        std::cout << "      Total Probability = " << total << std::endl;
    }
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;

    vector<double long>* nonAccept = mehr.findNonAccept(solExpectations, outcomeWorths, outcomeProbability);

    std::cout << nonAccept->at(0) << std::endl;
    std::cout << nonAccept->at(1) << std::endl;
    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == 0) {
            ASSERT_EQ((*nonAccept)[i], 1);
            break;
        }
    }
    for (int i=0; i < solSet->size(); ++i) {
        if (solSet->at(i)->policy[0][0] == 1) {
            ASSERT_EQ((*nonAccept)[i], 0.3);
            break;
        }
    }


    ASSERT_EQ((*nonAccept)[0], 1);
    ASSERT_EQ((*nonAccept)[1], 1);

}
