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
#include "MEHRPlan_lib/Logger.hpp"

class Runner {
    int stage=0;
public:
    shared_ptr<MDP> mdp;
    shared_ptr<Solver> solver;
    unique_ptr<MEHR> mehr;
    vector<vector<History*>> histories;
    vector<Policy*> policies;
    vector<double> non_accept;
    vector<QValue> polExpectations;
    std::string fileIn;

    Runner() = default;

    explicit Runner(const std::string& fileIn) {
        setInputFile(fileIn);
    }
    void setInputFile(const std::string& fileIn) {
        std::ifstream file(fileIn);
        if (!file.is_open()) {
            this->fileIn = DATA_FOLDER_PATH;
            this->fileIn += "/" + fileIn;
            file.open(this->fileIn);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open " + fileIn);
            }
        } else {
            this->fileIn = fileIn;
        }
        json data;
        try {
            data  = json::parse(file);
            file.close();
        }
        catch (json::parse_error& ex) {
            std::cout << "Cannot read file: " << ex.what() << std::endl;

            data  = json::parse(file);
            file.close();

        }
        mdp = make_shared<MDP>(data);


        solver = make_shared<Solver>(*mdp);
        stage = 1;
    }

    void solve() {
        timePlan();
        timeExtractSols();
        timeExtractHists();
        timeMEHR();
    }

    long long timePlan() {
        if (stage < 1) { throw std::runtime_error("Not Ready for plannng"); }
        long long d = timeFunc(&Solver::MOiLAO, *solver);
        Log::writeLog(std::format("Finished Planning in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);
        stage = 2;
        return d;
    }

    long long timeExtractSols() {
        if (stage < 2) { throw std::runtime_error("Not ready for Extract Solutions."); }
        long long d = timeFunc(&Solver::getSolutions, *solver, policies);
        Log::writeLog(std::format("Finished Extracting Solutions in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);
        stage = 3;
        return d;
    }

    long long timeExtractHists() {
        if (stage < 3) { throw std::runtime_error("Not ready for Extract Histories."); }
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
        stage = 4;
        return d;
    }

    long long timeMEHR() {
        if (stage < 4) { throw std::runtime_error("Not ready for MEHR."); }
        // Extract just the expectations of each policy
        polExpectations.reserve(policies.size());
        for (auto *pi : policies) {
            polExpectations.push_back(pi->worth.at(0));
        }
        // Create MEHR object
        mehr = make_unique<MEHR>(*mdp, histories, polExpectations, true);
        // Time and start MEHR:
        long long d = timeFunc(&MEHR::findNonAccept, *mehr, non_accept);

        Log::writeLog(mehr->ToString(polExpectations, histories, non_accept), LogLevel::Debug);
        Log::writeLog(std::format("Finished MEHR in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);
        stage = 5;
        return d;
    }
    void writeTo(std::string &fileOut) {
            auto durations = array<long long, 4>();

            // Do planning
            durations[0] = timePlan();

            // Extract Solutions
            durations[1] = timeExtractSols();

            // Extract histories
            durations[2] = timeExtractHists();

            durations[3] = timeMEHR();
            Log::writeLog(std::format("Total time {} {}", std::accumulate(durations.begin(), durations.end(), (long long)(0)), TIME_METRIC_STR), LogLevel::Info);

            //
            // Write results to file
            //
            auto outputter = Output(mdp.get(), *solver);
            outputter.addDuration(durations[0], durations[1], durations[2], durations[3]);
            outputter.addHistories(histories);
            outputter.addPolicies(policies, non_accept);
            outputter.writeToFile(fileOut, fileIn);
    }
};



#endif //RUNNER_HPP
