//
// Created by Simon Kolker on 06/09/2024.
//

#include "gtest/gtest.h"
#include "MDP.hpp"
#include "Utilitarianism.hpp"
#include "Solution.hpp"
#include <Solver.hpp>

class test_MO_PLAN : public ::testing::Test {
protected:
    void SetUp() {
    }

    MDP* MakeMDP(const std::string& fileName) {
        std::string dataFolder = DATA_FOLDER_PATH;
        std::string slash = "/";
        std::string fn = dataFolder + slash + fileName;
        return new MDP(fn);
    }
    void Horizon_1_Checks(MDP& mdp, std::vector<std::shared_ptr<Solution>>& solSet) {
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
    }

    void Level_2_Prune_Checks(MDP& mdp, std::vector<std::shared_ptr<Solution>>& solSet) {
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
    }

    void Level_3_UnPrune_Checks(MDP& mdp, std::vector<std::shared_ptr<Solution>>& solSet) {
        ASSERT_EQ(solSet.size(), 4);
        // TODO More vigorous checking...
    }
};

class MOVI_Test : public test_MO_PLAN {

};
/*
TEST_F(MOVI_Test, Horizon_1_Backup) {
    MDP* mdp = MakeMDP("my_test.json");
    Solver solver = Solver(*mdp);
    //std::vector<std::shared_ptr<Solution>> solSet = solver.MOValueIteration();
    //Horizon_1_Checks(*mdp, solSet);
    delete mdp;
}
TEST_F(MOVI_Test, Level_2_Prune) {
    MDP* mdp = MakeMDP("level_2_prune.json");
    Solver solver = Solver(*mdp);
    // std::vector<std::shared_ptr<Solution>> solSet = solver.MOValueIteration();
    // Level_2_Prune_Checks(*mdp, solSet);
    delete mdp;
}
TEST_F(MOVI_Test, Level_3_UnPrune) {
    MDP* mdp = MakeMDP("level_3_unPrune.json");
    Solver solver = Solver(*mdp);
    // std::vector<std::shared_ptr<Solution>> solSet = solver.MOValueIteration();
    // Level_3_UnPrune_Checks(*mdp, solSet);
    delete mdp;
}


TEST_F(MOVI_Test, LAO_Horizon_1_Backup) {
    MDP* mdp = MakeMDP("my_test.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    Horizon_1_Checks(*mdp, *solSet);
    delete mdp;
}
TEST_F(MOVI_Test, LAO_Level_2_Prune) {
    MDP* mdp = MakeMDP("level_2_prune.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    std::cout << solver.SolutionSetToString(*solSet);
    ASSERT_EQ(solSet->size(), 1);
    Solution* sol = solSet->at(0).get();

    ASSERT_EQ(sol->policy[0][0], 0);
    //ASSERT_EQ(sol->policy[1][0], 0);

    ExpectedUtility* eu_0 = dynamic_cast<ExpectedUtility*>(sol->expecters[0]->expectations[0][0]);
    ExpectedUtility* eu_1 = dynamic_cast<ExpectedUtility*>(sol->expecters[1]->expectations[0][0]);
    ASSERT_EQ(eu_0->value, 3);
    ASSERT_EQ(eu_1->value, 0);

    delete mdp;
}

TEST_F(MOVI_Test, LAO_Level_3_UnPrune) {
    MDP* mdp = MakeMDP("level_3_unPrune.json");
    Solver solver = Solver(*mdp);
    solver.MOiLAO();
    std::vector<std::shared_ptr<Solution>>* solSet = solver.extractSolutions();
    ASSERT_EQ(solSet->size(), 1);// TODO With admissible heuristic, correct answer would become 2.
    delete mdp;
}*/



