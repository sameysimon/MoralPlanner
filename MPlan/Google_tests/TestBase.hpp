//
// Created by Simon Kolker on 23/09/2024.
//

#ifndef TESTBASE_HPP
#define TESTBASE_HPP
#include "gtest/gtest.h"
#include "MDP.hpp"
#include "Utilitarianism.hpp"
#include "Solution.hpp"
#include <MEHR.hpp>


class TestBase : public ::testing::Test {
protected:
    void SetUp() {
    }
    MDP* MakeMDP(const std::string& fileName) {
        std::string dataFolder = DATA_FOLDER_PATH;
        std::string slash = "/";
        std::string fn = dataFolder + slash + fileName;
        return new MDP(fn);
    }
    void checkExpectedUtility(QValue& solValue, int theoryIndex, double utility, bool& found) {
        found = false;
        if (theoryIndex > solValue.expectations.size()) {
            FAIL() << "QValue has not got expectation for theory with index " << theoryIndex << ". Likely a testing error?";
        }
        ExpectedUtility* eu = dynamic_cast<ExpectedUtility*>(solValue.expectations[theoryIndex]);
        if (eu==nullptr) {
            FAIL() << "Should be an expected utility object.";
        }

        found = eu->value==utility;
    }
    bool checkPolicyStateInTime(Solution& sol, int stateID, vector<int>& actions) {
        for (int t=0; t<actions.size(); t++) {
            if (actions[t] != sol.policy[t][stateID]) {
                return false;
            }
        }
        return true;
    }
    bool checkContainsQValue(vector<QValue>* outcomeWorths, vector<double>* outcomeProbs, int theoryOne, int theoryTwo, double probability) {
        bool one = false;
        bool two = false;
        bool three;
        for (int i=0; i < outcomeWorths->size(); i++) {
            checkExpectedUtility(outcomeWorths->at(i), 0, theoryOne, one);
            checkExpectedUtility(outcomeWorths->at(i), 1, theoryTwo, two);
            three = outcomeProbs->at(i)==probability;
            if (one and two and three) {
                return true;
            }
        }
        return false;
    }
};



#endif //TESTBASE_HPP
