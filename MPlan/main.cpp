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
#include <curl/curl.h>
#include <numeric>
using json = nlohmann::json;
// DO NOT FORGET TO CHANGE BOTH OF BELOW
typedef chrono::milliseconds time_metric;
#define TIME_METRIC_STR "milliseconds"

int sendDataToServer() {
    nlohmann::json data = nlohmann::json::object();
    data["Hello"] = "World";
    CURL* curl;
    CURLcode res;
    // Initialize CURL
    curl = curl_easy_init();
    if(curl) {
        std::string jsonData = data.dump();
        curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:5000/data");
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            std::cerr << "Error: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return (res == CURLE_OK) ? 0 : 1;
    }
    return 1;
}

int main(int argc, const char * argv[]) {
    std::cout << "THE 2024 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string outputFolder = OUTPUT_FOLDER_PATH;

    std::string fileIn;
    std::string fileOut = outputFolder + "MPlan-Out.json";
    #ifdef DEBUG
    fileIn = dataFolder + "lostInsulin.json";
    fileIn = dataFolder + "CostCarlaSteal.json";
    fileOut = outputFolder + "Debug-MPlan-Out.json";
    #endif
    // Release
    if (argc == 3) {
        fileIn = argv[1];
        fileOut = argv[2];
    } else {
        std::cout << "Please provide an environment input file and and output file." << std::endl;
    }

    // Initialisation
    MDP* mdp = new MDP(fileIn);
    Solver solver = Solver(*mdp);
    MEHR mehr = MEHR(*mdp);
    auto durations = vector<long long>();
    chrono::time_point<chrono::steady_clock> start;
    chrono::time_point<chrono::steady_clock> end;
    long long d = 0;


    start = chrono::high_resolution_clock::now();
    solver.MOiLAO();
    end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();
#ifdef DEBUG
    std::cout << "Finished Planning in " << d << " " << TIME_METRIC_STR << ". " << std::endl;
#endif
    durations.push_back(d);

    start = chrono::high_resolution_clock::now();
    auto policies = solver.getSolutions();
    end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();
#ifdef DEBUG
    std::cout << "Finished Extracting solutions in " << d << " " << TIME_METRIC_STR << ". " <<  std::endl;
    std::cout << "Total of " << policies->size() << " solutions." <<  std::endl;
#endif
    durations.push_back(d);


    auto polExpectations = new vector<QValue>();
    auto eh = new ExtractHistories(*mdp);
    start = chrono::high_resolution_clock::now();
    auto histories = eh->extract(*policies, *polExpectations);
    end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();
#ifdef DEBUG
    std::cout << eh->ToString(*policies, *polExpectations, histories) << endl;
    std::cout << "Finished Extracting Histories in " << d << " " << TIME_METRIC_STR << ". " <<  std::endl;
#endif
    durations.push_back(d);

    start = chrono::high_resolution_clock::now();
    auto non_accept = mehr.findNonAccept(*polExpectations, histories);
    end = chrono::high_resolution_clock::now();
    d = chrono::duration_cast<time_metric>(end-start).count();
    std::cout << mehr.ToString(*polExpectations, histories) << std:: endl;
#ifdef DEBUG
    std::cout << "Finished MEHR in " << d << " " << TIME_METRIC_STR << ". " <<  std::endl;
#endif
    durations.push_back(d);

#ifdef DEBUG
    std::cout << "Final Time: " << std::accumulate(durations.begin(), durations.end(), (long long)(0)) << " " << TIME_METRIC_STR << ". " <<  std::endl;
    std::cout << "Finished Planning in " << durations[0] << " " << TIME_METRIC_STR << ". " <<  std::endl;
    std::cout << "Finished Extracting solutions in " << durations[1] << " " << TIME_METRIC_STR << ". " <<  std::endl;
    std::cout << "Finished Extracting Outcomes in " << durations[2] << " " << TIME_METRIC_STR << ". " <<  std::endl;
    std::cout << "Finished MEHR in " << durations[3] << " " << TIME_METRIC_STR << ". " <<  std::endl;
#endif
    Outputter::outputResults(mdp, non_accept, durations, *policies, fileOut, fileIn, solver);



    delete polExpectations;
    delete policies;
    delete eh;
    delete mdp;
    return 0;
}



