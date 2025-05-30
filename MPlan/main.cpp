//
//  main.cpp
//  MPlan
//
//  Created by Simon Kolker on 10/07/2024.
//
#include <iostream>
#include "MDP.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include "MEHRPlan_lib/Logger.hpp"
#include "REST_App.hpp"
#include "Runner.hpp"
using json = nlohmann::json;


/**
 *
 * @param argv [file_input] [file_output] [debug_level] | --server
 */
int main(int argc, const char * argv[]) {
    std::cout << "THE 2025 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    bool run_as_server = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0) {
            run_as_server = true;
        }
    }
    if (run_as_server) {
        auto app = REST_App();
        return 0;
    }

    std::string dataFolder = DATA_FOLDER_PATH;
    std::string outputFolder = OUTPUT_FOLDER_PATH;
    std::string fileIn = dataFolder + "../Experiments/LostInsulin/AAMAS Results/mdps/CostCarla_con0.json";
    std::string fileOut = outputFolder + "MPlan-Out.json";


    if (argc==0) {
        Log::writeLog("Using default input/output files...");
    }
    if (argc==4) {
        Log::setLogLevel(strtol(argv[3], nullptr, 10));
        Log::writeFormatLog(Info, "Debug level set to {}" , argv[3]);
    }
    if (argc >= 3) {
        fileIn = argv[1];
        fileOut = argv[2];
    }
    Log::writeLog(std::format("Chosen {} as input MDP file.", fileIn), LogLevel::Info);
    Log::writeLog(std::format("Chosen {} as output file.", fileOut), LogLevel::Info);
    Runner run = Runner(fileIn);
    run.writeTo(fileOut);

    return 0;
}



