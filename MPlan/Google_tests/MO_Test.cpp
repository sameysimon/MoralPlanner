//
// Created by Simon Kolker on 06/09/2024.
//

#include "gtest/gtest.h"
#include "MDP.hpp"
#include "Utilitarianism.hpp"
#include "Solution.hpp"
#include <Solver.hpp>

class MO_Test : public ::testing::Test {
protected:
    void SetUp() {
    }

    MDP* MakeMDP(const std::string& fileName) {
        std::string dataFolder = DATA_FOLDER_PATH;
        std::string fn = dataFolder + fileName;
        return new MDP(fn);
    }
};



TEST_F(MO_Test, Horizon_1_Backup) {
    MDP* mdp = MakeMDP("/my_test.json");
    Solver solver = Solver(*mdp);
    std::vector<std::shared_ptr<Solution>> solSet = solver.MOValueIteration();

    ASSERT_EQ(solSet.size(), 2);

    bool foundOne = false;
    bool foundTwo = false;

    // Check for first objective affirming policy.
    for (std::shared_ptr<Solution>& sol : solSet) {
        if (sol->policy[0][0] == 0) {
            ExpectedUtility* eu_0 = dynamic_cast<ExpectedUtility*>(sol->expecters[0]->expectations[0][0]);
            ExpectedUtility* eu_1 = dynamic_cast<ExpectedUtility*>(sol->expecters[1]->expectations[0][0]);
            if (eu_0->value==1 and eu_1->value==0) {
                foundOne = true;
                break; // Has a policy affirming first objective.

            }
        }
    }
    // Check for second objective affirming policy.
    for (std::shared_ptr<Solution>& sol : solSet) {
        if (sol->policy[0][0] == 1) {
            ExpectedUtility* eu_0 = dynamic_cast<ExpectedUtility*>(sol->expecters[0]->expectations[0][0]);
            ExpectedUtility* eu_1 = dynamic_cast<ExpectedUtility*>(sol->expecters[1]->expectations[0][0]);
            if (eu_0->value==0 and eu_1->value==1) {
                foundTwo = true;
                break; // Has a policy affirming first objective.
            }
        }
    }
    ASSERT_TRUE(foundOne and foundTwo);
    delete mdp;
}

TEST_F(MO_Test, Level_2_Prune) {
    MDP* mdp = MakeMDP("/level_2_prune.json");
    Solver solver = Solver(*mdp);
    std::vector<std::shared_ptr<Solution>> solSet = solver.MOValueIteration();

    ASSERT_EQ(solSet.size(), 2);
    bool foundOne = false;
    bool foundTwo = false;
    // Check for first objective affirming policy.

    for (std::shared_ptr<Solution>& sol : solSet) {
        if (sol->policy[0][0] == 0 and sol->policy[1][0] == 0) {
            ExpectedUtility* eu_0 = dynamic_cast<ExpectedUtility*>(sol->expecters[0]->expectations[0][0]);
            ExpectedUtility* eu_1 = dynamic_cast<ExpectedUtility*>(sol->expecters[1]->expectations[0][0]);
            if (eu_0->value==3 and eu_1->value==0) {
                eu_0 = dynamic_cast<ExpectedUtility*>(sol->expecters[0]->expectations[1][0]);
                eu_1 = dynamic_cast<ExpectedUtility*>(sol->expecters[1]->expectations[1][0]);
                if (eu_0->value==3 and eu_1->value==0) {
                    foundOne = true;
                    break; // Has a policy affirming first objective.
                }
            }
        }
    }
    for (std::shared_ptr<Solution>& sol : solSet) {
        if (sol->policy[0][0] == 0 and sol->policy[1][0] == 1) {
            ExpectedUtility* eu_0 = dynamic_cast<ExpectedUtility*>(sol->expecters[0]->expectations[0][0]);
            ExpectedUtility* eu_1 = dynamic_cast<ExpectedUtility*>(sol->expecters[1]->expectations[0][0]);
            if (eu_0->value==3 and eu_1->value==0) {
                eu_0 = dynamic_cast<ExpectedUtility*>(sol->expecters[0]->expectations[1][0]);
                eu_1 = dynamic_cast<ExpectedUtility*>(sol->expecters[1]->expectations[1][0]);
                if (eu_0->value==0 and eu_1->value==1) {
                    foundTwo = true;
                    break; // Has a policy affirming first objective.
                }
            }
        }

    }
    ASSERT_TRUE(foundOne);
    ASSERT_TRUE(foundTwo);
    delete mdp;
}



