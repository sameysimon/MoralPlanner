//
// Created by e56834sk on 23/08/2024.
//
#include "gtest/gtest.h"
#include "MDP.hpp"
#include "Utilitarianism.hpp"
#include "Solution.hpp"
class QValueTestFixture : public ::testing::Test {

};

TEST_F(QValueTestFixture, EvaluateUtilityQValues) {
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string fn = dataFolder + "/test.json";
    MDP mdp = MDP(fn);
    Solution sol = Solution(mdp);
    State* s = mdp.states[0];
    Action* a1 = mdp.actions[0];
    ASSERT_EQ(*a1->label, "A");
    Action* a2 = mdp.actions[1];
    ASSERT_EQ(*a2->label, "B");

    std::vector<Successor*>* successors = mdp.getActionSuccessors(*s, 0);
    QValue qv = sol.vectorGather(*successors, 0);
    ExpectedUtility* eu = static_cast<ExpectedUtility*>(qv.expectations[0]);
    ASSERT_EQ(eu->value, 7.5);

    successors = mdp.getActionSuccessors(*s, 1);
    qv = sol.vectorGather(*successors, 0);
    eu = dynamic_cast<ExpectedUtility*>(qv.expectations[0]);
    ASSERT_EQ(eu->value, 1);
}

TEST_F(QValueTestFixture, CompareLexiEqualQValues) {
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string fn = dataFolder + "/test.json";
    MDP mdp = MDP(fn);
    Solution sol = Solution(mdp);
    State* s = mdp.states[0];
    Action* a1 = mdp.actions[0];
    ASSERT_EQ(*a1->label, "A");
    Action* a2 = mdp.actions[1];
    ASSERT_EQ(*a2->label, "B");

    std::vector<Successor*>* successors = mdp.getActionSuccessors(*s, 0);
    QValue qv1 = sol.vectorGather(*successors, 0);
    ExpectedUtility* eu1 = dynamic_cast<ExpectedUtility*>(qv1.expectations[0]);
    ASSERT_EQ(eu1->value, 7.5);

    successors = mdp.getActionSuccessors(*s, 1);
    QValue qv2 = sol.vectorGather(*successors, 0);
    ExpectedUtility* eu2 = dynamic_cast<ExpectedUtility*>(qv2.expectations[0]);
    ASSERT_EQ(eu2->value, 1);


    ASSERT_EQ(mdp.compareQValues(qv1, qv2), 1);
    ASSERT_EQ(mdp.compareQValues(qv2, qv1), -1);
    ASSERT_EQ(mdp.compareQValues(qv1, qv1), 0);
    ASSERT_EQ(mdp.compareQValues(qv2, qv2), 0);



}