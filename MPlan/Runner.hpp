//
// Created by Simon Kolker on 24/03/2025.
//

#ifndef RUNNER_HPP
#define RUNNER_HPP

#include "fstream"
#include "time_config.hpp"
#include "MDP.hpp"
#include "Solver.hpp"
#include "MEHR.hpp"
#include <chrono>
#include "ExtractHistories.hpp"
#include "MEHRPlan_lib/Logger.hpp"
#include "nlohmann/json.hpp"
#include "JSONBuilder.hpp"


struct explainResult {
    long long planTime=0;
    long long solutionExtractionTime=0;
    long long historyExtractionTime=0;
    long long mehrTime=0;
    long long totalTime=0;
    std::vector<size_t> newPolicyIndices;
};

class Runner {
    int stage=0;
public:
    array<long long, 4> durations;
    shared_ptr<MDP> mdp;
    shared_ptr<Solver> solver;
    unique_ptr<MEHR> mehr;
    vector<vector<History*>> histories;
    vector<Policy*> policies;
    shared_ptr<NonAcceptability> non_accept;
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
        if (stage < 1) { throw std::runtime_error("Not Ready for planning"); }
        long long d = timeFunc(&Solver::MOiAO_Star, *solver);
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

        Log::writeLog(eh->ToString(policies, *polExpectations, histories), LogLevel::Debug);
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
        mehr = make_unique<MEHR>(*mdp, policies, histories);
        non_accept = make_shared<NonAcceptability>(mdp->mehr_theories.size(), policies.size());
        // Time and start MEHR:
        long long d = timeFunc(&MEHR::findNonAccept, *mehr, *non_accept);

        Log::writeLog(mehr->ToString(polExpectations, histories, *non_accept), LogLevel::Debug);
        Log::writeLog(std::format("Finished MEHR in {} {}.", d, TIME_METRIC_STR), LogLevel::Info);
        stage = 5;
        return d;
    }
    void writeTo(std::string &fileOut) {
            // Do planning
            durations[0] = timePlan();

            // Extract Solutions
            durations[1] = timeExtractSols();

            // Extract histories
            durations[2] = timeExtractHists();

            durations[3] = timeMEHR();
            Log::writeLog(std::format("Total time {} {}", std::accumulate(durations.begin(), durations.end(), (long long)(0)), TIME_METRIC_STR), Info);

            // Save File
            json result = JSONBuilder::toJSON(*this);
            std::ofstream file(fileOut);
            if (file.is_open()) {
                file << result.dump(4);
                file.close();
                std::cout << "JSON file '" << fileOut << "' created successfully!" << std::endl;
            }
            else {
                std::cerr << "Could not open the file '" << fileOut << "' for output." << std::endl;
            }
    }

    explainResult explain(size_t stateIdx, size_t act_idx, Policy* policy_base) {
        explainResult r{};
        auto satisfyingPols = mdp->findPoliciesWithAction(policies, *mdp->states[stateIdx], (int)act_idx);
        if (!satisfyingPols.empty()) {
            r.newPolicyIndices = satisfyingPols;
            return r;
        }
        State* pState = mdp->states[stateIdx];
        // Plan
        solver->lockAction(*pState, act_idx, *policy_base);
        r.planTime = timeFunc(&Solver::MOiAO_Star, *solver);
        // Get solutions
        vector<Policy*> newPolicies;
        r.solutionExtractionTime = timeFunc(&Solver::getSolutions, *solver, newPolicies);
        // Get histories
        auto eh = new ExtractHistories(*mdp);
        vector<vector<History*>> newHistories;
        r.historyExtractionTime = timeFunc(&ExtractHistories::extract, *eh, newHistories, newPolicies);

        // Do MEHR
        r.newPolicyIndices.resize(newPolicies.size());
        std::iota(r.newPolicyIndices.begin(), r.newPolicyIndices.end(), policies.size());
        r.mehrTime = timeFunc(&MEHR::addPoliciesToMEHR, *mehr, *non_accept, newPolicies, newHistories);
        r.totalTime = r.solutionExtractionTime + r.historyExtractionTime + r.mehrTime + r.planTime;

        solver->removeLocks();
        return r;
    }

    explainResult explain(size_t stateIdx, std::string action_label, Policy* policy_base) {
        State *pState = mdp->states[stateIdx];
        // Find action with state.
        auto actions = mdp->getActions(*pState);
        size_t act_idx = -1;
        for (size_t i = 0; i < actions->size(); ++i) {
            if (actions->at(i)->label == action_label) {
                act_idx = i;
                break;
            }
        }
        if (act_idx == -1) {
            throw runtime_error(std::format("Explain: Could not find action with label {} for state id {}", action_label, stateIdx));
        }
        return explain(stateIdx, act_idx, policy_base);
    }
};



#endif //RUNNER_HPP
