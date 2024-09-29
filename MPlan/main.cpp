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
using json = nlohmann::json;

// Function to sort and get indices in ascending order
std::vector<size_t> sort_indices(const std::vector<double>& non_accept) {
    // Create a vector of indices (0, 1, 2, ..., non_accept.size() - 1)
    std::vector<size_t> indices(non_accept.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }
    // Sort indices based on comparing the values in non_accept
    std::sort(indices.begin(), indices.end(),
              [&non_accept](size_t i1, size_t i2) { return non_accept[i1] < non_accept[i2]; });

    return indices;
}


int main(int argc, const char * argv[]) {
    std::cout << "THE 2024 MACHINE ETHICS HYPOTHETICAL RETROSPECTION PLANNER (MEHR-PLAN)" << std::endl;
    std::string dataFolder = DATA_FOLDER_PATH;
    std::string fn = dataFolder + "/my_test.json";

    if (false) {//argc == 1) {
        fn = argv[0];
        std::cout << "Please provide an environment file." << std::endl;
        return 0;
    }
    if (argc > 2)
        fn = argv[1];

    // Initialisation
    MDP* mdp = new MDP(fn);
    Solver solver = Solver(*mdp);
    MEHR mehr = MEHR(*mdp);

    // Timing and operation
    auto start = chrono::high_resolution_clock::now();
    vector<shared_ptr<Solution>> candidates = solver.MOiLAO();
    auto non_accept = mehr.solve(*mdp, candidates);
    auto end = chrono::high_resolution_clock::now();

    // Output results
    auto duration = chrono::duration_cast<chrono::microseconds>(end-start);
    std::cout << "Finished in " << duration.count() << " microseconds." <<  std::endl;
    std::vector<size_t> sorted_indices = sort_indices(*non_accept);
    int counter = 0;
    for (auto i : sorted_indices) {
        std::cout << counter << std::endl;
        std::cout << "Non-Acceptability=" << (*non_accept)[i] << std::endl;
        std::cout <<  (candidates)[i]->worthToString() << std::endl;
        std::cout <<  (candidates)[i]->policyToString() << std::endl << std::endl;
        counter++;
    }



    return 0;
}



