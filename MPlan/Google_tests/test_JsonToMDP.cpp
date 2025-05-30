//
// Created by e56834sk on 21/08/2024.
//
#include "TestBase.hpp"
#include "MDP.hpp"
#include <fstream>
#include <unordered_map>
#include <algorithm>

using json = nlohmann::json;

class JsonMDPFixture : public TestBase {
  protected:

    static void CheckForSuccessor(std::vector<Successor*>& successors, int source, int target, double prob) {
        for (auto & successor : successors) {
            if ((successor->source==source and successor->target==target) and (successor->probability==prob)) {
                return;
            }
        }
        FAIL() << "No successor with source " << source << ", target " << target << ", probability " << prob << ".";
    }
};

TEST_F(JsonMDPFixture, ConstructorTest) {
    MDP mdp = *getMDP("test.json");

    ASSERT_EQ(mdp.states.size(), 2);
    ASSERT_EQ(mdp.total_states, 2);
    ASSERT_EQ(mdp.mehr_theories.size(), 1);
    ASSERT_EQ(mdp.considerations.size(), 1);

    // Check state 1 is the goal:
    ASSERT_EQ(mdp.states[1]->isGoal, true);

    // Check states map to correct actions.
    auto acts = mdp.getActions(*mdp.states[0]);
    bool hasActionA = std::any_of(acts->begin(), acts->end(), [&](shared_ptr<Action>& a) {
        return a->label == "A";
    });
    ASSERT_TRUE(hasActionA);
    bool hasActionB = std::any_of(acts->begin(), acts->end(), [&](shared_ptr<Action>& a) {
        return a->label == "B";
    });
    ASSERT_TRUE(hasActionB);


    // Check for successors:
    // Action A, state 0
    std::vector<Successor*>* pSuccessors = MDP::getActionSuccessors(*mdp.states[0], 0);
    CheckForSuccessor(*pSuccessors, 0, 1, 0.5);
    CheckForSuccessor(*pSuccessors, 0, 1, 0.5);

    // Action B, state 0
    pSuccessors = MDP::getActionSuccessors(*mdp.states[0], 1);
    CheckForSuccessor(*pSuccessors, 0, 1, 1);

}



