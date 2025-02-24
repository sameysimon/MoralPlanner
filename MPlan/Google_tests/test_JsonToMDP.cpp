//
// Created by e56834sk on 21/08/2024.
//
#include "gtest/gtest.h"
#include "MDP.hpp"
#include <fstream>
#include <unordered_map>
#include <algorithm>

using json = nlohmann::json;

class JsonMDPFixture : public ::testing::Test {
  protected:
    template <typename T>
    bool checkPointerBy(std::vector<T>& objs, T& target) {
        return std::find(objs.begin(), objs.end(), target) != objs.end();
    }
    void checkForSuccessor(std::vector<Successor*>& successors, int source, int target, double prob) {

        for (int i = 0; i < successors.size(); i++) {
            if ((successors[i]->source==source and successors[i]->target==target) and (successors[i]->probability==prob)) {
                return;
            }
        }
        FAIL() << "No successor with source " << source << ", target " << target << ", probability " << prob << ".";
    }
};

TEST_F(JsonMDPFixture, ConstructorTest) {
    /*
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string fn = dataFolder + "/test.json";
    MDP mdp = MDP(fn);
    EXPECT_EQ(mdp.states.size(), 2);
    EXPECT_EQ(mdp.total_states, 2);
    EXPECT_EQ(mdp.theories.size(), 1);

    // Check there are correct actions
    EXPECT_EQ(mdp.actions.size(), 3);
    EXPECT_EQ(*mdp.actions[0]->label, "A");
    Action* actA = mdp.actions[0];
    EXPECT_EQ(*mdp.actions[1]->label, "B");
    Action* actB = mdp.actions[1];
    EXPECT_EQ(*mdp.actions[2]->label, "C");
    Action* actC = mdp.actions[2];

    // Check state 1 is the goal:
    EXPECT_EQ(mdp.states[1]->isGoal, true);

    // Check states go to correct actions.
    std::vector<Action*> acts = *mdp.getActions(*mdp.states[0]);
    // State 1 should go to Action A and Action B.
    EXPECT_EQ(checkPointerBy(acts, actA), true);
    EXPECT_EQ(checkPointerBy(acts, actB), true);


    // Check for successors:
    // Action A, state 0
    std::vector<Successor*> successors;
    successors = *mdp.getActionSuccessors(*mdp.states[0], 0);
    checkForSuccessor(successors, 0, 1, 0.5);
    checkForSuccessor(successors, 0, 1, 0.5);

    // Action B, state 0
    successors = *mdp.getActionSuccessors(*mdp.states[0], 1);
    checkForSuccessor(successors, 0, 1, 1);*/

}



