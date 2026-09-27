//
// Created by Simon Kolker on 24/03/2025.
//

#pragma once

#include "fstream"
#include "time_config.hpp"
#include "MDP.hpp"
#include "Solver.hpp"
#include "MEHR.hpp"
#include "ExtractSolutions.hpp"
#include <chrono>
#include "History.hpp"
#include "Logger.hpp"
#include "nlohmann/json.hpp"
#include "JSONBuilder.hpp"

struct Durations {
    long long heuristicTime = 0;
    long long planTime=0;
    long long solutionExtractionTime=0;
    long long historyExtractionTime=0;
    long long solutionReductionTime = 0;
    long long mehrTime=0;

    [[nodiscard]] long long Total() const {
        return heuristicTime + planTime + solutionExtractionTime + historyExtractionTime + solutionReductionTime + mehrTime;
    }

};
struct explainResult {
    long long planTime=0;
    long long solutionExtractionTime=0;
    long long solutionReductionTime=0;
    long long historyExtractionTime=0;
    long long mehrTime=0;
    std::vector<size_t> newPolicyIndices;
    [[nodiscard]] long long getTotalTime() const {
        return planTime + solutionExtractionTime + solutionReductionTime + historyExtractionTime + mehrTime;
    }
};
enum Solver_Stage {
    START=0,
    INIT_MDP,
    DONE_HEURISTIC,
    DONE_PLANING,
    DONE_SOLUTION_EXTRACTION,
    DONE_MEHR
};
enum Planning_Mode {
    DOMAIN_HEURISTIC=0,
    INDEPENDENT_HEURISTIC,
    MCDP
};

class Runner {
    Solver_Stage stage = START;
public:
    Durations durations;
    shared_ptr<MDP> mdp;
    shared_ptr<Solver> solver;
    unique_ptr<SolutionExtracter> soln_extractor;
    unique_ptr<MEHR> mehr;

    vector<vector<unique_ptr<History>>> histories;
    vector<unique_ptr<Policy>> policies;
    size_t undominated_policies = 0;
    shared_ptr<NonAcceptability> non_accept;
    vector<QValue> polExpectations;
    std::string fileIn;
    bool make_history_paths = false;

    Planning_Mode planning_mode = INDEPENDENT_HEURISTIC;

    Runner() = default;
    explicit Runner(const std::string& fileIn_, bool make_history_paths_ = true) {
        make_history_paths = make_history_paths_;
        SetInputFile(fileIn_);
    }
    int SetInputFile(const std::string& inputFile) {
        fileIn = inputFile;
        auto file = OpenFile(fileIn);
        json data;
        try {
            data  = json::parse(file);
            file.close();
        }
        catch (json::parse_error& ex) {
            Log::writeLog(std::format("Error parsing MMMDP: {}", ex.what()), LogLevel::Error);
            file.close();
            return EXIT_FAILURE;
        }
        mdp = make_shared<MDP>(data);
        solver = make_shared<Solver>(*mdp, planning_mode == DOMAIN_HEURISTIC);
        stage = INIT_MDP;
        return EXIT_SUCCESS;
    }
    static std::ifstream OpenFile(const std::string& fn) {
        const std::vector<std::string> paths = {
            fn,
            "/" + fn,
            std::string(DATA_FOLDER_PATH) + fn
        };

        std::string tries;
        for (const auto& path : paths) {
            std::ifstream file(path);
            if (file.is_open()) {
                return file;
            }
            if (!tries.empty()) {
                tries += "\n";
            }
            tries += path;
        }

        throw std::runtime_error("Could not open file. Tried:\n" + tries + "\n");
    }

    [[nodiscard]] string MakePoliciesString() const {
        string s;
        for (size_t i=0; i < policies.size(); ++i) {
            auto &pi = *policies[i];
            s += format("Policy {} expects {}\n", i, pi.getExpectationPtr()->toString());
            double pr=0;
            for (size_t j = 0; j < histories[i].size(); ++j) {
                s += format("   History {} expects {} at p={}", j, histories[i][j]->mWorth.toString(), histories[i][j]->probability);
                if (histories[i][j]->hasPath) {
                    s += "w/ path [";
                    for (auto st : *histories[i][j]->path) {
                        s += format(" {}, ", st);
                    }
                    s+= "]\n";
                } else {
                    s += "\n";
                }
                pr+=histories[i][j]->probability;
            }
            s += format("Total Probability = {}\n\n", pr);
        }
        return s;
    }

    long long timeHeuristic() {
        if (stage < INIT_MDP) { throw std::runtime_error("No MDP yet."); }
        long long d = 0;
        if (planning_mode == INDEPENDENT_HEURISTIC) {
            Log::writeLog(std::format("Starting build heuristic..."), LogLevel::Info);
            d = CPUTime(&Solver::BuildIndependentHeuristic, *solver);
            Log::writeLog(std::format("Finished Building Heuristic in {} {}.", d, TIME_METRIC_STR), LogLevel::Warn);
        } else {
            Log::writeLog(std::format("Skipped building Heuristic in {} {}.", d, TIME_METRIC_STR), LogLevel::Warn);
        }
        stage = DONE_HEURISTIC;
        return d;
    }

    long long timePlan() {
        long long d = 0;
        if (planning_mode==MCDP) {
            Log::writeLog(std::format("Starting planning..."), LogLevel::Info);
            d = CPUTime(&Solver::MCDP, *solver);
        } else {
            Log::writeLog(std::format("Starting heuristic planning..."), LogLevel::Info);
            d = CPUTime(&Solver::MC_iAO_Star, *solver);
        }
        Log::writeLog(std::format("Finished Planning in {} {}", d, TIME_METRIC_STR), LogLevel::Warn);
        stage = DONE_PLANING;
        return d;
    }

    long long timeExtractSols(bool prune_dominated = true) {
        if (stage < DONE_PLANING) { throw std::runtime_error("Not ready for Extract Solutions."); }
        Log::writeLog(std::format("Starting extract solutions..."), LogLevel::Info);
        soln_extractor = make_unique<SolutionExtracter>(*mdp, make_history_paths);
        soln_extractor->prune_dominated = prune_dominated;
        long long d = CPUTime(&SolutionExtracter::Extract, *soln_extractor, policies, histories, solver->mPi, solver->mBackupOrder);
        undominated_policies = policies.size();
#ifdef DEBUG
        Log::writeFormatLog(LogLevel::Debug, "{}", MakePoliciesString());
        Log::writeFormatLog(LogLevel::Debug, "{}", soln_extractor->stringify(policies, *mdp));
#endif
        Log::writeLog(std::format("Extracted {} policies in {} {}.", policies.size(), d, TIME_METRIC_STR), LogLevel::Warn);
        stage = DONE_SOLUTION_EXTRACTION;
        return d;
    }

    long long timeMEHR() {
        if (stage < DONE_SOLUTION_EXTRACTION) { throw std::runtime_error("Not ready for MEHR."); }
        Log::writeLog(std::format("Starting MEHR..."), LogLevel::Info);
        // Create MEHR object
        mehr = make_unique<MEHR>(*mdp, policies, histories);
        non_accept = make_shared<NonAcceptability>(mdp->mehr_theories.size(), policies.size());
        // Time and start MEHR:
        long long d = CPUTime(&MEHR::Slow_FindNonAccept, *mehr, *non_accept);
        if (Log::getLogLevel() >= LogLevel::Debug) {
            Log::writeLog(mehr->ToString(*non_accept), LogLevel::Debug);
        }
        Log::writeLog(std::format("Finished MEHR in {} {}.", d, TIME_METRIC_STR), LogLevel::Warn);
        stage = DONE_MEHR;
        return d;
    }

    void Plan_Only(std::string &fileOut) {
        durations.heuristicTime = timeHeuristic();
        durations.planTime = timePlan();
        Log::writeLog(std::format("Total time {} {}", durations.Total(), TIME_METRIC_STR), LogLevel::Info);
        json result;
        result.merge_patch(JSONBuilder::toJSON(durations));
        result.merge_patch(JSONBuilder::addInputJSON(fileIn));
        WriteJSONFile(result, fileOut);
    }
    void MEHR_Only(const std::string &fileOut) {
        // Selected actions are all available actions.
        solver->mPi = vector<vector<int>>(mdp->states.size());
        solver->mPi[0].resize(mdp->getActions(*mdp->states[0])->size(),0);
        iota(solver->mPi[0].begin(), solver->mPi[0].end(), 0);
        // The selected actions form the plan that solution extraction consumes.
        stage = DONE_PLANING;
        durations.solutionExtractionTime = timeExtractSols(false);
        durations.mehrTime = timeMEHR();

        if (fileOut=="-1") {
            return;
        }
        json result = JSONBuilder::toJSON(*this);
        WriteJSONFile(result, fileOut);
    }

    void FullSolve() {
        durations.heuristicTime = timeHeuristic();
        durations.planTime = timePlan();
        durations.solutionExtractionTime = timeExtractSols();
        durations.mehrTime = timeMEHR();
    }
    void FullSolve(const std::string &fileOut) {
        FullSolve();
        Log::writeLog(std::format("Total time {} {}", durations.Total(), TIME_METRIC_STR), LogLevel::Info);
        // Save File
        json result = JSONBuilder::toJSON(*this);
        WriteJSONFile(result, fileOut);
    }

    static void WriteJSONFile(json &data, const std::string &fileOut) {
        std::ofstream file(fileOut);
        if (file.is_open()) {
            file << data.dump(4);
            file.close();
            std::cout << "JSON file '" << fileOut << "' created successfully!" << std::endl;
        }
        else {
            std::cerr << "Could not open the file '" << fileOut << "' for output." << std::endl;
        }
    }

    explainResult PlanLocked(size_t stateIdx, size_t act_idx, Policy* policy_base) {
        explainResult r{};
        auto satisfyingPols = mdp->findPoliciesWithAction(policies, *mdp->states[stateIdx], (int)act_idx);
        if (!satisfyingPols.empty()) {
            r.newPolicyIndices = satisfyingPols;
            return r;
        }
        State* pState = mdp->states[stateIdx];
        // Plan
        solver->ResetExpansion();
        solver->lockAction(*pState, act_idx, *policy_base);
        r.planTime = CPUTime(&Solver::MC_iAO_Star, *solver);

        // Get Policies
        vector<unique_ptr<Policy>> newPolicies;
        policy_hists newHistories;
        vector<int> bo = solver->mBackupOrder;
        r.solutionExtractionTime = CPUTime(&SolutionExtracter::Extract, *soln_extractor, newPolicies, newHistories, solver->mPi, bo);
        r.newPolicyIndices = vector<size_t>(newPolicies.size());
        for (size_t piIdx = 0; piIdx < newPolicies.size(); ++piIdx) {
            policies.emplace_back(std::move(newPolicies[piIdx]));
            histories.emplace_back(std::move(histories[piIdx]));
            r.newPolicyIndices[piIdx] = piIdx + newPolicies.size();
        }
        solver->removeLocks();
        return r;
    }

    vector<QValue> EvaluateAction(size_t stateIdx, size_t act_idx, Policy* policy_base) {
        vector<QValue> policyWorth;
        auto satisfyingPols = mdp->findPoliciesWithAction(policies, *mdp->states[stateIdx], (int)act_idx);
        policyWorth.reserve(satisfyingPols.size());
        for (size_t piIdx : satisfyingPols) {
            policyWorth.push_back(*policies[piIdx]->getExpectationPtr());
        }
        if (!policyWorth.empty()) {
            return policyWorth;
        }
        State* pState = mdp->states[stateIdx];
        // Plan
        solver->UseTempData(true);
        solver->ResetExpansion();
        solver->lockAction(*pState, act_idx, *policy_base);
        CPUTime(&Solver::MC_iAO_Star, *solver);
        policyWorth = solver->GetQValuesAtState(0);
        solver->UseTempData(false);
        solver->removeLocks();
        return policyWorth;
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
        solver->ResetExpansion();
        solver->lockAction(*pState, act_idx, *policy_base);
        r.planTime = CPUTime(&Solver::MC_iAO_Star, *solver);
        // Get solutions
        vector<unique_ptr<Policy>> newPolicies;
        policy_hists newHistories;
        soln_extractor->ForceStateAction(stateIdx, act_idx);
        r.solutionExtractionTime = CPUTime(&SolutionExtracter::Extract, *soln_extractor, newPolicies, newHistories, solver->mPi, solver->mBackupOrder);

        // Do MEHR
        size_t old_policies_size = policies.size();
        r.newPolicyIndices = vector<size_t>(newPolicies.size());
        for (size_t piIdx = 0; piIdx < newPolicies.size(); ++piIdx) {
            r.newPolicyIndices[piIdx] = piIdx + old_policies_size;
            policies.emplace_back(std::move(newPolicies[piIdx]));
            histories.emplace_back(std::move(newHistories[piIdx]));
        }
        r.mehrTime = CPUTime(&MEHR::addPoliciesToMEHR, *mehr, *non_accept, r.newPolicyIndices);
        solver->removeLocks();
        return r;
    }
    explainResult explain(size_t stateIdx, string act, Policy* policy_base) {
        auto actions = mdp->getActions(*mdp->states[stateIdx]);
        int act_idx;
        for (act_idx = 0; act_idx<actions->size(); act_idx++) {
            if (actions->at(act_idx)->label==act) {
                break;
            }
        }
        return explain(stateIdx, act_idx, policy_base);
    }
};
