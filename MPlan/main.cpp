//
//  main.cpp
//  MPlan
//
//  Created by Simon Kolker on 10/07/2024.
//
#include <iostream>
#include "MDP.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include "Solver.hpp"
using json = nlohmann::json;

int main(int argc, const char * argv[]) {
    std::cout << "THE 2024 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string fn = dataFolder + "/newDomain.json";
    if (argc == 2)
        fn = argv[1];

    MDP* mdp = new MDP(fn);
    Solver solver = Solver(*mdp);
    //solver.valueIteration();

    solver.MOValueIteration();

    return 0;
}



