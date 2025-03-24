//
// Created by Simon Kolker on 23/09/2024.
//

#ifndef TESTBASE_HPP
#define TESTBASE_HPP
#include "gtest/gtest.h"
#include "MDP.hpp"
#include "Utilitarianism.hpp"

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

};



#endif //TESTBASE_HPP
