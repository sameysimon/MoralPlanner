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
        if (theoryIndex > solValue.expectations.size()) {
            FAIL() << "QValue has not got expectation for theory with index " << theoryIndex << ". Likely a testing error?";
        }
        ExpectedUtility* eu = dynamic_cast<ExpectedUtility*>(solValue.expectations[theoryIndex]);
        if (eu==nullptr) {
            FAIL() << "Should be an expected utility object.";
        }
        found = eu->value==utility;

    }
    bool checkPolicyAtTime(shared_ptr<Solution> sol, int state, int time, int action) {
        return sol->policy[time][state]==action;
    }
    bool checkContainsQValue(shared_ptr<vector<QValue>> outcomeWorths, shared_ptr<vector<double long>> outcomeProbs, int theoryOne, int theoryTwo, double long probability) {
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
