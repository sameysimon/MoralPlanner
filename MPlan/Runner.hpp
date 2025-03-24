//
// Created by Simon Kolker on 24/03/2025.
//

#ifndef RUNNER_HPP
#define RUNNER_HPP


#include "time_config.hpp"
#include "MDP.hpp"
#include "Solver.hpp"
#include "MEHR.hpp"
#include <chrono>
#include "Output.hpp"
#include "ExtractHistories.hpp"
#include "Logger.hpp"


class Runner {
    int state=0;
public:
    unique_ptr<MDP> mdp;
    unique_ptr<Solver> solver;
    vector<vector<History*>> histories;
    vector<Policy*> policies;
    vector<double> non_accept;
    vector<QValue> polExpectations;

    explicit Runner(const std::string& fileIn) {
        try {
            mdp = make_unique<MDP>(fileIn);
        }
        catch (const std::runtime_error& e) {
            try {
                mdp = make_unique<MDP>(DATA_FOLDER_PATH + fileIn);
            }
            catch (const std::runtime_error& e1) {
                throw std::runtime_error(e.what());
            }
        }
        solver = make_unique<Solver>(*mdp);
        state = 1;
    }
    void solve() {
        timePlan();
        timeExtractSols();
        timeExtractHists();
        timeMEHR();
    }

    long long timePlan() {
        if (state < 1) { throw std::runtime_error("Not Ready for plannng"); }
        long long d = timeFunc(&Solver::MOiLAO, *solver);
        Log::writeLog(std::format("Finished Planning in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);
        state = 2;
        return d;
    }

    long long timeExtractSols() {
        if (state < 2) { throw std::runtime_error("Not ready for Extract Solutions."); }
        long long d = timeFunc(&Solver::getSolutions, *solver, policies);
        Log::writeLog(std::format("Finished Extracting Solutions in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);
        state = 3;
        return d;
    }

    long long timeExtractHists() {
        if (state < 3) { throw std::runtime_error("Not ready for Extract Histories."); }
        auto eh = new ExtractHistories(*mdp);
        long long d = timeFunc(&ExtractHistories::extract, *eh, histories, policies);
        Log::writeLog(std::format("Finished Extracting Histories in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);

        auto polExpectations = new vector<QValue>();
        polExpectations->reserve(policies.size());
        for (auto *pi : policies) {
            polExpectations->push_back(pi->worth.at(0));
        }

        Log::writeLog(eh->ToString(policies, *polExpectations, histories));
        delete eh;
        delete polExpectations;
        state = 4;
        return d;
    }

    long long timeMEHR() {
        if (state < 4) { throw std::runtime_error("Not ready for MEHR."); }
        // Extract just the expectations of each policy
        polExpectations.reserve(policies.size());
        for (auto *pi : policies) {
            polExpectations.push_back(pi->worth.at(0));
        }
        // Create MEHR object
        MEHR mehr = MEHR(*mdp, histories, polExpectations, true);
        // Time and start MEHR:
        long long d = timeFunc(&MEHR::findNonAccept, mehr, non_accept);

        Log::writeLog(mehr.ToString(polExpectations, histories, non_accept), LogLevel::Debug);
        Log::writeLog(std::format("Finished MEHR in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);
        state = 5;
        return d;
    }
};



#endif //RUNNER_HPP
