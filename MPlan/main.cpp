//
//  main.cpp
//  MPlan
//
//  Created by Simon Kolker on 10/07/2024.
//
#include <iostream>
#include "time_config.hpp"
#include "MDP.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include "Output.hpp"
#include "ExtractHistories.hpp"
#include <numeric>
#include "Logger.hpp"
#include "Runner.hpp"
using json = nlohmann::json;

int main(int argc, const char * argv[]) {
    //
    // Handle command line arguments
    //
    std::cout << "THE 2024 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string outputFolder = OUTPUT_FOLDER_PATH;
    std::string fileIn = dataFolder + "combos.json";
    std::string fileOut = outputFolder + "MPlan-Out.json";

    if (argc >= 3) {
        if (argc==4) {
            // Sets debug level.
            Log::setLogLevel(strtol(argv[3], nullptr, 10));
        }
        fileIn = argv[1];
        fileOut = argv[2];
        Log::writeLog(std::format("Chosen {} as input MDP file.", fileIn), LogLevel::Info);
        Log::writeLog(std::format("Chosen {} as output file.", fileOut), LogLevel::Info);

    } else {
        std::cout << "Please provide an environment input file and an output file." << std::endl;
    }

    //
    //  Do the stuff!
    //
    Runner runner = Runner(fileIn);
    auto durations = array<long long, 4>();

    // Do planning
    durations[0] = runner.timePlan();

    // Extract Solutions
    durations[1] = runner.timeExtractSols();

    // Extract histories
    durations[2] = runner.timeExtractHists();

    durations[3] = runner.timeMEHR();
    Log::writeLog(std::format("Total time {} {}", std::accumulate(durations.begin(), durations.end(), (long long)(0)), TIME_METRIC_STR), LogLevel::Info);

    //
    // Write results to file
    //
    Output outputter = Output(runner.mdp.get(), *runner.solver);
    outputter.addDuration(durations[0], durations[1], durations[2], durations[3]);
    outputter.addHistories(runner.histories);
    outputter.addPolicies(runner.policies, runner.non_accept);
    outputter.writeToFile(fileOut, fileIn);

    return 0;
}



