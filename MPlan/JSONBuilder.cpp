//
// Created by Simon Kolker on 15/07/2025.
//
#include "JSONBuilder.hpp"
#include "Runner.hpp"
#include "Utilitarianism.hpp"
#include "Solver.hpp"
#include <string>

#include "Policy_Counter.h"

json JSONBuilder::toJSON(Runner& run) {
    json result = json::object();
    result.merge_patch(toJSON(run.durations));
    result.merge_patch(toJSON(run.histories, false));
    result.merge_patch(toJSON(*(run.solver)));
    result.merge_patch(toJSON(run.policies, *(run.mdp), *(run.non_accept)));

    result[FIELD::TOTAL_ATTACKS] = run.mehr->GetTotalAttacks();
    result[FIELD::DURATION_CQ_1] = run.mehr->cq1_time;
    result[FIELD::DURATION_CQ_2] = run.mehr->cq2_time;

    result.merge_patch(addInputJSON(run.fileIn));

    return result;
}

// Add input file to end of json for easy processing!
json JSONBuilder::addInputJSON(const std::string &fileIn) {
    json r = json::object();
    std::ifstream inputFile = std::ifstream(fileIn);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Error loading input file: '" + fileIn + "'");
    }
    json inFile = json::parse(inputFile);
    r.merge_patch(inFile);
    return r;
}

json JSONBuilder::toJSON(const std::vector<Attack>& attackVector) {
    json r = json::array();
    short attackIdx=0;
    for (auto &att : attackVector) {
        for (auto e : att.HistoryEdges) {
            r[attackIdx] = json::object();
            r[attackIdx][FIELD::SOURCEPOLICY] = att.sourcePolicyIdx;
            r[attackIdx][FIELD::TARGETPOLICY] = att.targetPolicyIdx;
            r[attackIdx][FIELD::SOURCEHISTORY] = e.first;
            r[attackIdx][FIELD::TARGETHISTORY] = e.second;
            r[attackIdx][FIELD::THEORY] = att.theoryIdx;
            attackIdx++;
        }
    }
    return r;
}

json JSONBuilder::toJSON(explainResult &er, Runner& runner, bool addMEHR) {
    json r = json::object();
    // Add durations
    r.merge_patch(toJSON(runner.durations));

    // Add Policies + Histories
    r[FIELD::FOILSOLUTIONS] = json::object();
    r[FIELD::HISTORIES] = json::object();
    r[FIELD::ATTACKS] = json::object();
    for (size_t i : er.newPolicyIndices) {
        double nacc = -1;
        if (addMEHR) {
            nacc = runner.non_accept->getPolicyNonAccept(i);;
        }
        r[FIELD::FOILSOLUTIONS][std::to_string(i)] = toJSON(*runner.policies[i], *(runner.mdp), nacc, runner.histories.at(i));
        r[FIELD::HISTORIES][std::to_string(i)] = json::array();
        for (auto &h : runner.histories[i]) {
            r[FIELD::HISTORIES][std::to_string(i)].push_back(toJSON(*h));
        }
        r[FIELD::ATTACKS][std::to_string(i)] = toJSON(runner.mehr->attacks[i]);
    }
    return r;
}

json JSONBuilder::toJSON(vector<unique_ptr<Policy>>& policies, MDP &mdp, NonAcceptability &non_accept, bool includeActions) {
    json ar = json::array();
    std::vector<size_t> sorted_indices = sort_indices(non_accept);
    double min_non_acc = non_accept.getPolicyNonAccept(sorted_indices[0]);
    int num_of_min_non_acc = 0;
    for (size_t i =0; i < policies.size(); ++i) {
        auto nacc = non_accept.getPolicyNonAccept(i);
        ar.push_back(toJSON(*policies[i], mdp, nacc, includeActions));
        if (nacc==min_non_acc) {
            num_of_min_non_acc++;
        }

    }
    json r = json::object();
    r[FIELD::SOLUTIONS] = ar;
    r[FIELD::SOLUTION_TOTAL] = policies.size();
    r[FIELD::SOLUTIONS_ORDER] = sorted_indices;
    r[FIELD::NUM_OF_MIN_NON_ACCEPT] = num_of_min_non_acc;
    CountPolicies c(mdp);
    r[FIELD::TOTAL_POLICIES] = c.GetCount();

    return r;
}

json JSONBuilder::toJSON(Policy &pi, MDP &mdp, double non_accept, bool includeActions) {
    json r = json::object();
    // Set non-acceptability
    r[FIELD::NON_ACCEPTABILITY] = non_accept;

    // Build action map
    if (includeActions) {
        json action_map = json::object();
        buildPolicyMap(action_map, pi, mdp);
        r[FIELD::ACTION_MAP] = action_map;
    }
    // Add Expected Cost
    if (mdp.non_moralTheoryIdx != -1) {
        ExpectedUtility* cost = dynamic_cast<ExpectedUtility*>(pi.worth[0].expectations[mdp.non_moralTheoryIdx].get());
        r[FIELD::EXPECTED_COST] = cost->value;
    }

    // Add policy's expectations for all moral theories.
    r[FIELD::EXPECTATION] = json::object();
    for (int thIdx=0; thIdx < mdp.considerations.size(); thIdx++) {
        r[FIELD::EXPECTATION][mdp.considerations[thIdx]->label] = pi.worth[0].expectations[thIdx]->ToString();
    }
    return r;
}
json JSONBuilder::toJSON(Policy &pi, MDP &mdp, double non_accept, vector<unique_ptr<History>>& policy_histories) {
    json r = toJSON(pi, mdp, non_accept);
    r[FIELD::TOTAL_HISTORIES] = policy_histories.size();
    QValue min_qv = policy_histories[0]->mWorth;
    QValue max_qv = policy_histories[0]->mWorth;

    for (size_t i = 0; i < mdp.considerations.size(); ++i) {
        for (auto &h_vec : policy_histories) {
            if (min_qv.expectations[i]->compare(*h_vec->mWorth.expectations[i]) == -1) {
                *min_qv.expectations[i] = *h_vec->mWorth.expectations[i];
            }
            if (max_qv.expectations[i]->compare(*h_vec->mWorth.expectations[i]) == 1) {
                *max_qv.expectations[i] = *h_vec->mWorth.expectations[i];
            }
        }
    }
    r[FIELD::MIN_HISTORIES] = toJSON(min_qv);
    r[FIELD::MAX_HISTORIES] = toJSON(max_qv);
    return r;
}

json JSONBuilder::toJSON(policy_hists& histories, bool includePaths) {
    json r = json::object();
    size_t max=0;
    size_t min=histories[0].size();
    unsigned long avg=0;
    for (const auto& hist : histories) {
        avg+=hist.size();
        max = std::max(max, hist.size());
        min = std::min(min, hist.size());
    }
    avg = avg / (unsigned long)histories.size();
    r[FIELD::AVG_HISTORIES] = avg;
    r[FIELD::MIN_HISTORIES] = min;
    r[FIELD::MAX_HISTORIES] = max;
    r[FIELD::HISTORIES] = json::object();
    int piIdx = 0;
    if (includePaths) {
        for (auto &piHists : histories) {
            json policyHistories = json::array();
            for (auto &h : piHists) {
                policyHistories.push_back(toJSON(*h));
            }
            r[FIELD::HISTORIES][piIdx] = policyHistories;
            piIdx++;
        }
    }
    return r;
}
json JSONBuilder::toJSON(History& h) {
    // {probability: float, worth: string, path: [s_1, s_2, ... s_H]
    json r = json::object();
    r[FIELD::PROBABILITY] = h.probability;
    r[FIELD::WORTH] = h.mWorth.toString();
    r[FIELD::PATH] = toJSON(*h.path);
    return r;
}

json JSONBuilder::toJSON(vector<size_t>& path) {
    vector<size_t> path_(path.size()+1, 0);
    for (size_t i=0; i < path.size(); i++) {
        path_[i+1] = path[path.size() - 1 - i];
    }
    path_[0] = 0;
    return path_;
}

json JSONBuilder::toJSON(Durations& dur) {
    json r;
    r[FIELD::DURATION_HEURISTIC] = dur.heuristicTime;
    r[FIELD::DURATION_PLAN] = dur.planTime;
    r[FIELD::DURATION_SOLS] = dur.solutionExtractionTime;
    r[FIELD::DURATION_SOLS_REDUCE] = dur.solutionReductionTime;
    r[FIELD::DURATION_OUTS] = dur.historyExtractionTime;
    r[FIELD::DURATION_MEHR] = dur.mehrTime;
    r[FIELD::DURATION_TOTAL] = dur.Total();
    return r;
}
json JSONBuilder::toJSON(QValue& qv) {
    json r = json::array();
    r = qv.toStringVector();
    return r;
}


json JSONBuilder::toJSON(Solver& solver) {
    json r;
    r[FIELD::EXPANDED_STATES] = solver.expanded_states;
    r[FIELD::BSG_SIZE] = solver.mPi.size();
    r[FIELD::BACKUPS] = solver.backups;
    r[FIELD::ITERATIONS] = solver.expansions;
    return r;
}

json JSONBuilder::toJSON(vector<QValue>& candidates, vector<int>& indicesOfUndominated, vector<int>& qValueIdxToAction, MDP& mdp, State& s) {
    json r;
    // Make an object + fields for each action
    for (int i=0; i<candidates.size(); i++) {
        std::string act = (mdp.getActions(s))->at(qValueIdxToAction[i])->label;
        r[act] = json::object();
        r[act]["containsUndominated"] = false;
        r[act]["QValues"] = json::array();
        r[act]["indexOfUndominated"] = indicesOfUndominated;
    }
    for (int i=0; i<candidates.size(); i++) {
        std::string act = (mdp.getActions(s))->at(qValueIdxToAction[i])->label;
        // Add qvalue to action.
        r[act]["QValues"][r[act]["QValues"].size()] = json::object();
        r[act]["QValues"][r[act]["QValues"].size()-1]["Value"] = candidates[i].toStringVector();
        r[act]["QValues"][r[act]["QValues"].size()-1]["isUndominated"] = std::find(indicesOfUndominated.begin(), indicesOfUndominated.end(), i)!=indicesOfUndominated.end();
        r[act]["containsUndominated"] = r[act]["containsUndominated"].dump()=="true" || std::find(indicesOfUndominated.begin(), indicesOfUndominated.end(), i)!=indicesOfUndominated.end();
    }
    return r;
}