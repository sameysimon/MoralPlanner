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
    std::cout << "THE 2026 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    bool run_as_server = false;
    bool have_file_input = false;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string outputFolder = OUTPUT_FOLDER_PATH;
    std::string fileIn = dataFolder + "../Experiments/Random/2025-03-21 16:27:23/mdps/0Util_0Law__hor=6_con0.json";
    fileIn = "/Users/user/Desktop/MyMoralPlanner/MoralPlanner/Data/Experiments/Elder/2026-01-15 11:56:16/mdps/priv=auto=health_con0_rep0.json";
    fileIn = "/Users/user/Desktop/MyMoralPlanner/MoralPlanner/Data/Experiments/Elder/2026-01-16 13:58:33/mdps/0_con0.json";
    std::string fileOut = outputFolder + "MPlan-Out.json";
    Log::setLogLevel(LogLevel::Warn);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 || strcmp(argv[i], "-S") == 0) {
            run_as_server = true;
        }
        else if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-D") == 0) {
            if (i >= argc - 1) {
                std::cout << "--debug option requires an argument" << std::endl;
            }
            Log::setLogLevel(strtol(argv[i+1], nullptr, 10));
            Log::writeFormatLog(Info, "Debug level set to {}" , argv[3]);
            i++;
        }
        else if (!have_file_input) {
            fileIn = argv[i];
            have_file_input = true;
        } else {
            fileOut = argv[i];
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-H") == 0) {
            std::cout << "Usage: MPlan [options] [mmmdp_file_input] [mmmdp_file_output]" << std::endl;
            std::cout << "  options:" << std::endl;
            std::cout << "  -H, --help          Shows help" << std::endl;
            std::cout << "  -S, --server        Run in server mode" << std::endl;
            std::cout << "  -D, --debug [level] Set debug level" << std::endl;
        }
    }

    if (run_as_server) {
        auto app = REST_App();
        return 0;
    }

    Log::writeLog(std::format("Chosen {} as input MDP file.", fileIn), LogLevel::Info);
    Log::writeLog(std::format("Chosen {} as output file.", fileOut), LogLevel::Info);
    Runner run = Runner();
    int x = run.SetInputFile(fileIn);
    if (x==1) {
        return x;
    }
    run.WriteTo(fileOut);
    return 0;

}



