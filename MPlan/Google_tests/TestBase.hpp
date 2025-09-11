//
// Created by Simon Kolker on 23/09/2024.
//

#ifndef TESTBASE_HPP
#define TESTBASE_HPP
#include "gtest/gtest.h"
#include "MDP.hpp"
#include "Utilitarianism.hpp"
#include "Solver.hpp"
#include "../Runner.hpp"

#include <MEHR.hpp>

#include "Solver.hpp"


class TestBase : public ::testing::Test {
protected:
    void SetUp() {
    }
    static MDP* getMDP(const std::string& fileName) {
        std::string dataFolder = DATA_FOLDER_PATH;
        std::string fn = dataFolder + fileName;
        return new MDP(fn);
    }
    // Given a runner with policies, builds list of policy indices.
    // return vector policies[i] has is such that runner.policies[policies[i]](state_idx) = actions[i]
    // In other words, policy id policies[i] has state_idx map to actions[i].
    static vector<size_t> getPolicyIdsByStateAction(Runner& r, int state_idx, vector<string> &actions) {
        auto stateActions = r.mdp->getActions(*r.mdp->states[state_idx]);
        // Map action string to Action Idx.
        vector<size_t> actionToIdx = getActionIdsByLabels(r, state_idx, actions);
        // Match policies to action Idx.
        vector<size_t> actionToPolicyIdx(actions.size(), -1);
        for (int i=0; i < r.policies.size(); ++i) {
            for (size_t aIdx=0; aIdx<actionToIdx.size(); ++aIdx) {
                if (r.policies.at(i)->policy[state_idx] == actionToIdx[aIdx]) {
                    actionToPolicyIdx[aIdx] = i;
                }
            }
        }
        return actionToPolicyIdx;
    }

    static vector<size_t> getActionIdsByLabels(Runner& r, int state_idx, vector<string> &labels) {
        auto stateActions = r.mdp->getActions(*r.mdp->states[state_idx]);
        // Map action string to Action Idx.
        vector<size_t> actionToIdx(labels.size(), -1);
        for (int i = 0; i < stateActions->size(); ++i) {
            for (size_t aIdx=0; aIdx<labels.size(); ++aIdx) {
                if (stateActions->at(i)->label == labels[aIdx]) {
                    actionToIdx[aIdx] = i;
                }
            }
        }
        return actionToIdx;
    }
};





#endif //TESTBASE_HPP
