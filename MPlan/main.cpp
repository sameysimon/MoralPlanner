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
    bool plan_only = false;
    Planning_Mode pm = Planning_Mode::INDEPENDENT_HEURISTIC;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string outputFolder = OUTPUT_FOLDER_PATH;
    std::string fileIn = dataFolder + "../Experiments/Random/2025-03-21 16:27:23/mdps/0Util_0Law__hor=6_con0.json";
    ushort portIn = 18080;
    fileIn = "/Users/user/Desktop/MyMoralPlanner/MoralPlanner/Data/Experiments/SearchRescue/2026-06-30_12:47:51/mdps/Search_Rescue_con0.json";
    std::string fileOut = outputFolder + "MPlan-Out.json";
    Log::setLogLevel(LogLevel::Debug);

    if (argc==1) {
        std::cout << "Call with a Multi-Moral Markov Decision Process/Stochastic Shortest Path JSON file!" << std::endl;
    }
#ifdef DEBUG
    Log::setLogLevel(LogLevel::Debug);
#endif
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 || strcmp(argv[i], "-S") == 0) {
            run_as_server = true;
        }
        else if (strcmp(argv[i], "--plan_only") == 0 || strcmp(argv[i], "-PO") == 0) {
            plan_only = true;
        }
        else if (strcmp(argv[i], "--plan_mode") == 0 || strcmp(argv[i], "-PM") == 0) {
            if (i >= argc - 1) {
                std::cout << "--plan_mode option requires an argument" << std::endl;
                return 0;
            }
            string mode = argv[i + 1];
            if (mode == "independent") {
                pm = INDEPENDENT_HEURISTIC;
            } else if (mode == "domain") {
                pm = DOMAIN_HEURISTIC;
            } else if (mode == "mcdp") {
                pm = MCDP;
            } else {
                Log::writeFormatLog(LogLevel::Info, "Planning mode {} does not exist." , mode);
                return 0;
            }
            Log::writeFormatLog(LogLevel::Info, "Planning mode set to {}" , mode);

        }
        else if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-D") == 0) {
            if (i >= argc - 1) {
                std::cout << "--debug option requires an argument" << std::endl;
                return 0;
            }
            uint8_t ll = strtol(argv[i+1], nullptr, 10);
            Log::setLogLevel(static_cast<LogLevel>(ll));
            Log::writeFormatLog(LogLevel::Info, "Debug level set to {}" , argv[3]);
            i++;
        } else if (run_as_server && strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-P") == 0) {
            if (i >= argc - 1) {
                std::cout << "--port option requires an argument" << std::endl;
                return 0;
            }
            portIn = strtol(argv[i+1], nullptr, 10);
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
            std::cout << "  Options:" << std::endl;
            std::cout << "  -H, --help          Shows help" << std::endl;
            std::cout << "  -S, --server     Run in server mode" << std::endl;
            std::cout << "  -P, --port     For server mode, specify port to listen on." << std::endl;
            std::cout << "  -PO, --plan_only     Plan only mode skips argumentation and policy extraction.." << std::endl;
            std::cout << "  -PM, --plan_mode [mode]    Planning modes include 'mcdp', 'independent' and 'domain'" << std::endl;
            std::cout << "  -D, --debug [level] Set debug level" << std::endl;
        }
    }

    if (run_as_server) {
        if (have_file_input) {
            auto app = REST_App(portIn, fileOut);
        } else {
            auto app = REST_App(portIn);
        }

        return 0;
    }
    Log::writeLog(std::format("Chosen {} as input MDP file.", fileIn), LogLevel::Info);
    Log::writeLog(std::format("Chosen {} as output file.", fileOut), LogLevel::Info);
    Runner run = Runner();
    run.planning_mode = pm;
    run.make_history_paths=true;
    int x = run.SetInputFile(fileIn);
    if (x==1) {
        return x;
    }
    if (plan_only) {
        run.Plan(fileOut);
    } else {
        run.FullSolve(fileOut);
    }

    return 0;

}



