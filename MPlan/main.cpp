//
//  main.cpp
//  MPlan
//
//  Created by Simon Kolker on 10/07/2024.
//
#include <iostream>
#include "MDP.hpp"
#include <nlohmann/json.hpp>
#include "Solver.hpp"
#include "MEHR.hpp"
#include <chrono>
#include <fstream>
#include "Outputter.hpp"
using json = nlohmann::json;




int main(int argc, const char * argv[]) {
    std::cout << "THE 2024 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string outputFolder = DATA_FOLDER_PATH;


    std::string fileIn;
    fileIn = dataFolder + "/lostInsulin.json";
    fileIn = dataFolder + "/windyDrone.json";
    std::string fileOut = outputFolder + "MPlan-Out.json";


    if (false and argc == 1) {
        fileIn = argv[0];
        std::cout << "Please provide an environment file." << std::endl;
        return 0;
    }
    if (argc == 2)
        fileIn = argv[1];

    // Initialisation
    MDP* mdp = new MDP(fileIn);
    Solver solver = Solver(*mdp);
    MEHR mehr = MEHR(*mdp);
    std::cout << "Initialised Environment in " << fileIn << ". Beginning Planning..." <<  std::endl;


    // Timing and operation
    std::cout << "Starting planning..." << std::endl;
    auto start = chrono::high_resolution_clock::now();
    solver.MOiLAO();
    auto end = chrono::high_resolution_clock::now();
    auto durationPlan = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    std::cout << "Finished Planning in " << durationPlan << " milliseconds." <<  std::endl;

    std::cout << "Extracting Solutions..." << std::endl;
    start = chrono::high_resolution_clock::now();
    auto candidates = solver.extractSolutions();
    end = chrono::high_resolution_clock::now();
    auto durationExtraction = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    std::cout << "Finished Extracting solutions in " << durationExtraction << " milliseconds." <<  std::endl;


    start = chrono::high_resolution_clock::now();
    auto non_accept = mehr.solve(*mdp, candidates);
    end = chrono::high_resolution_clock::now();
    auto durationMEHR = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    std::cout << "Finished MEHR in " << durationMEHR << " milliseconds." <<  std::endl;

    Outputter::outputResults(mdp, non_accept, durationPlan, durationMEHR, candidates, fileOut);

    return 0;
}



