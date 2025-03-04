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
#include "ExtractHistories.hpp"
#include <numeric>
using json = nlohmann::json;
// DO NOT FORGET TO CHANGE BOTH OF BELOW
typedef chrono::microseconds time_metric;
#define TIME_METRIC_STR "microseconds"

int main(int argc, const char * argv[]) {
    std::cout << "THE 2024 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string outputFolder = OUTPUT_FOLDER_PATH;
    int debugLevel = 1;
    std::string fileIn;
    std::string fileOut = outputFolder + "MPlan-Out.json";

    if (argc >= 3) {
        fileIn = argv[1];
        fileOut = argv[2];
        std::cout << "Chosen '" << fileIn << "' input file." << std::endl;
        std::cout << "Chosen '" << fileOut << "' as output file." << std::endl;
        if (argc==4) {
            // Sets debug level.
            debugLevel = atoi(argv[3]);
        }
    } else {
        std::cout << "Please provide an environment input file and an output file." << std::endl;
    }

    // Initialisation
    MDP* mdp = new MDP(fileIn);
    Solver solver = Solver(*mdp);
    MEHR mehr = MEHR(*mdp);
    auto durations = vector<long long>();
    
    long long d = 0;


    auto start = chrono::high_resolution_clock::now();
    solver.MOiLAO();
    auto end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();
    if (debugLevel>=1) {
        std::cout << "Finished Planning in " << d << " " << TIME_METRIC_STR << ". " << std::endl;
    }
    durations.push_back(d);

    //
    // Extract solutions
    //
    start = chrono::high_resolution_clock::now();
    auto policies = solver.getSolutions();
    end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();
    if (debugLevel>=1) {
        std::cout << "Finished Extracting solutions in " << d << " " << TIME_METRIC_STR << ". " <<  std::endl;
    }
    durations.push_back(d);

    //
    // Extract histories
    //
    auto eh = new ExtractHistories(*mdp);
    start = chrono::high_resolution_clock::now();
    auto histories = eh->extract(*policies);
    end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();

    // Gets stats on number of histories.
    std::array<float, 3> histStats = {0, 0, 0};
    for (const auto& hist : histories) {
        histStats[0]+=hist.size();
        histStats[1] = std::max(histStats[1], (float)hist.size());
        histStats[2] = std::min(histStats[2], (float)hist.size());
    }
    histStats[0] /= histories.size();

    if (debugLevel>=1) {
        std::cout << "Finished Extracting Histories in " << d << " " << TIME_METRIC_STR << ". " <<  std::endl;
    }
    durations.push_back(d);

    // Pull only the policy's expectations.
    auto polExpectations = new vector<QValue>();
    polExpectations->reserve(policies->size());
    for (auto *pi : *policies) {
        polExpectations->push_back(pi->worth.at(0));
    }
#ifdef DEBUG
    std::cout << eh->ToString(*policies, *polExpectations, histories) << endl;
#endif

    start = chrono::high_resolution_clock::now();
    auto non_accept = mehr.findNonAccept(*polExpectations, histories);
    end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();
#ifdef DEBUG
    std::cout << mehr.ToString(*polExpectations, histories) << std:: endl;
#endif
    if (debugLevel>=1) {
        std::cout << "Finished MEHR in " << d << " " << TIME_METRIC_STR << ". " <<  std::endl;
    }
    durations.push_back(d);
    if (debugLevel>=1) {
        std::cout << "Final Time: " << std::accumulate(durations.begin(), durations.end(), (long long)(0)) << " " << TIME_METRIC_STR << ". " <<  std::endl;
    }

    Outputter::outputResults(mdp, non_accept, durations, *policies, histStats, policies->size(), fileOut, fileIn, solver);

    delete polExpectations;
    delete policies;
    delete eh;
    delete mdp;
    return 0;
}



