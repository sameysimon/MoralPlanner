//
// Created by Simon Kolker on 08/04/2025.
//

#include "REST_App.hpp"
#include "JSONBuilder.hpp"
#include <algorithm>

inline bool isDoubleEqual(double d1, double d2) {
    return abs(d1 - d2) < DBL_EPSILON;
}

crow::response REST_App::HandleMDP(const crow::request &req) {
    Log::writeLog("MDP Request", Info);
    finishedSolving = false;// prevent other requests

    auto json_req = crow::json::load(req.body);
    string file_in;
    string file_out;
    std::string inp = DATA_FOLDER_PATH;
    std::string of = OUTPUT_FOLDER_PATH;
    bool use_data_file = true;
    try {
        file_in = json_req["file_in"].s();
        if (json_req.has("file_out")) {
            file_out = json_req["file_out"].s();
        } else {
            file_out = format("{}ServerRequest.json", OUTPUT_FOLDER_PATH);
        }
        if (json_req.has("use_data_file")) {
            use_data_file = json_req["use_data_file"].b();
        }
    } catch (runtime_error &e) {
        Log::writeLog(e.what(), Fatal);
        finishedSolving = true;
        return {500, e.what()};
    }

    runner = make_unique<Runner>(file_in);
    runner->make_history_paths = true;

    runner->WriteTo(file_out);
    json resp;
    resp["file_out"] = file_out;
    finishedSolving = true;
    return {resp.dump()};
}

crow::response REST_App::HandleQueryFoilAction(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (auto resp = BasicCheckValidRequest(json_req)) {
        return std::move(*resp);
    }
    if (!json_req.has("state_id")) {
        return {400, "Invalid Query Request: No state_id int."};
    }
    if (!json_req.has("action_label")) {
        return {400, "Invalid Query Request: No action label string."};
    }

    // Get request values
    size_t state_id;
    string action_label;
    size_t factPolicy_id;
    try {
        state_id = json_req["state_id"].i();
        action_label = json_req["action_label"].s();
        factPolicy_id = json_req["factPolicyIdx"].i();
    } catch (runtime_error &e) {
        std::cout << e.what() << "\n";
        finishedSolving = true;
        return {500, e.what()};
    }
    auto action_id = runner->mdp->getActionIndex(state_id, action_label);

    // Check if counter-factual is actually counterfactual
    auto &fact_pi = runner->policies[factPolicy_id];
    auto &isa = fact_pi->included_state_actions;
    nlohmann::json bod = nlohmann::json::object();
    if (isa.end() != find(isa.begin(), isa.end(), pair(state_id, action_id))) {
        bod["type"] = "Equivalent";
        return {200, bod.dump()};
    }

    // Check if counter-factual is actually preferred.
    bool isAnyCounterPreferred = false;
    bool oneCounterEqual = false;
    double factNacc = runner->non_accept->getPolicyNonAccept(factPolicy_id);
    for (size_t pi_i = 0; pi_i < runner->policies.size(); ++pi_i) {
        auto &pi = runner->policies[pi_i];
        if (pi->policy[static_cast<int>(state_id)] == action_id) {
            double counterNacc= runner->non_accept->getPolicyNonAccept(pi_i);
            if (counterNacc < factNacc) {
                isAnyCounterPreferred = true;
            }
            if (isDoubleEqual(counterNacc, factNacc)) {
                oneCounterEqual = true;
            }
        }
    }
    if (isAnyCounterPreferred) {
        bod["type"] = "False by MEHR";
        return {200, bod.dump()};
    }
    // If we are on the most pref'd policy, and at least one counter is same pref, then user pref.
    if (oneCounterEqual && isDoubleEqual(runner->non_accept->getMinimumNonAccept(), factNacc)) {
        bod["type"] = "User preference";
        return {200, bod.dump()};
    }

    vector<QValue> candidates = vector<QValue>();// The QValue candidates (corresponding to state-actions)
    vector<int> indicesOfUndominated = vector<int>();// Indices of candidate QValues that are undominated.
    vector<int> qValueIdxToAction = vector<int>();// Maps QValue index to action index.
    State* s = runner->mdp->states[state_id];
    auto actions = runner->mdp->getActions(*s);
    runner->solver->getUnDomCandidates(*s, candidates, indicesOfUndominated, qValueIdxToAction);
    bool anyInBudget = false;
    bool anyUndominated = false;
    for (size_t qvIdx = 0; qvIdx < candidates.size(); qvIdx++) {
        if (actions->at(qValueIdxToAction[qvIdx])->label != action_label) {
            continue;
        }
        if (runner->mdp->isQValueInBudget(candidates[qvIdx])) {
            anyInBudget = true;
        }
        if (find(indicesOfUndominated.begin(),indicesOfUndominated.end(),qvIdx) != indicesOfUndominated.end()) {
            anyUndominated = true;
        }
    }

    if (anyInBudget && anyUndominated) {
        bod["type"] = "MEHR Preference";
    }
    if (anyInBudget && !anyUndominated) {
        bod["type"] = "Pareto Dominance";
    }
    if (!anyInBudget && anyUndominated) {
        bod["type"] = "Non-moral";
    }
    if (!anyInBudget && !anyUndominated) {
        bod["type"] = "Pareto Dominance and Non-moral";
    }
    return {200, bod.dump()};
}

crow::response REST_App::HandleQValues(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (auto resp = BasicCheckValidRequest(json_req)) {
        return std::move(*resp);
    }
    if (!json_req.has("state_id")) {
        return {400, "Invalid QValue Request: No state_id int."};
    }
    int state_id = (int)json_req["state_id"].i();

    // Values we need
    vector<QValue> candidates = vector<QValue>();// The QValue candidates (corresponding to state-actions)
    vector<int> indicesOfUndominated = vector<int>();// Indices of candidate QValues that are undominated.
    vector<int> qValueIdxToAction = vector<int>();// Maps QValue index to action index.
    // Get the values
    State* s = runner->mdp->states[state_id];
    runner->solver->getUnDomCandidates(*s, candidates, indicesOfUndominated, qValueIdxToAction);
    return {200, JSONBuilder::toJSON(candidates, indicesOfUndominated, qValueIdxToAction, *runner->mdp, *s).dump()};

}

crow::response REST_App::HandleGetPolicyAttacks(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (auto resp = BasicCheckValidRequest(json_req)) {
        return std::move(*resp);
    }
    size_t policyIdx;
    try {
        policyIdx = (int)json_req["policyIdx"].i();
    } catch (runtime_error &err) {
        std::stringstream ss;
        ss << err.what();
        ss << "Failed to read request." << std::endl;
        std::cout << ss.str() << "\n";
        return {400, ss.str()};
    }
    if (policyIdx > runner->mehr->attacks.size()) {
        return {400, std::format("Invalid request for MEHR attacks. Policy with index {} does not exist.", policyIdx)};
    }

    return {200, JSONBuilder::toJSON(runner->mehr->attacks[policyIdx]).dump()};
}


crow::response REST_App::HandlePlanLocked(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (auto resp = BasicCheckValidRequest(json_req)) {
        return std::move(*resp);
    }
    if (!json_req.has("state_id")) {
        return {400, "Invalid Query Request: No state_id int."};
    }
    size_t state_id;
    std::string action_label;
    size_t factPolicy_id;
    try {
        state_id = (int)json_req["state_id"].i();
        action_label = json_req["actionLabel"].s();
        factPolicy_id = (int)json_req["factPolicyIdx"].i();
    } catch (runtime_error &err) {
        std::stringstream ss;
        ss << err.what();
        ss << "Failed to read request." << std::endl;
        std::cout << ss.str() << "\n";
        return {400, ss.str()};
    }
    auto actionIdx = runner->mdp->getActionIndex(state_id, action_label);
    if (!actionIdx) {
        return {400, format("No action '{}' on state with id {}", action_label, state_id)};
    }
    // No MEHR solution
    auto er = runner->PlanLocked(state_id, actionIdx.value(), runner->policies[factPolicy_id].get());
    return {200, JSONBuilder::toJSON(er, *runner, false).dump()};
}

crow::response REST_App::HandleExplain(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (auto resp = BasicCheckValidRequest(json_req)) {
        return std::move(*resp);
    }

    size_t state_id;
    std::string action_label;
    size_t factPolicy_id;
    try {
        state_id = (int)json_req["state_id"].i();
        action_label = json_req["actionLabel"].s();
        factPolicy_id = (int)json_req["factPolicyIdx"].i();
    } catch (runtime_error &err) {
        std::stringstream ss;
        ss << err.what();
        ss << "Failed to read request." << std::endl;
        std::cout << ss.str() << "\n";
        return {400, ss.str()};
    }

    auto actionIdx = runner->mdp->getActionIndex(state_id, action_label);
    if (!actionIdx) {
        return {400, format("No action '{}' on state with id {}", action_label, state_id)};
    }
    explainResult er = runner->explain(state_id, actionIdx.value(), runner->policies[factPolicy_id].get());

    return {200, JSONBuilder::toJSON(er, *runner, true).dump()};
}

crow::response REST_App::HandleMEHR(const crow::request &req) {
    std::cout << "MEHR REQUEST \n";
    auto json_req = crow::json::load(req.body);
    if (!finishedSolving) { return {400, "No MDP yet. Use /MDP to pass a problem file."}; }
    if (!json_req) { return {400, "Invalid JSON request"}; }

    auto policy_ids = json_req["policy_ids"].lo();

    crow::json::wvalue resp_payload;
    for (const auto& policy_id : policy_ids) {
        int policy_id_int = (int)policy_id.i();
        resp_payload[policy_id_int] = crow::json::wvalue(crow::json::type::List);
        for (Attack& att : runner->mehr->attacks[policy_id_int] ) {
            for (auto edge : att.HistoryEdges) {
                ushort idx = resp_payload[policy_id_int].size();
                resp_payload[policy_id_int][idx] = crow::json::wvalue(crow::json::type::Object);
                resp_payload[policy_id_int][idx]["sourceHistory"] = edge.first;
                resp_payload[policy_id_int][idx]["sourcePolicy"] = att.sourcePolicyIdx;
                resp_payload[policy_id_int][idx]["targetHistory"] = edge.second;
                resp_payload[policy_id_int][idx]["probability"] = runner->histories[att.targetPolicyIdx][edge.second]->probability;
                resp_payload[policy_id_int][idx]["sourceHistoryQValue"] = runner->histories[att.sourcePolicyIdx][edge.first]->mWorth.toString();
                resp_payload[policy_id_int][idx]["targetHistoryQValue"] = runner->histories[att.targetPolicyIdx][edge.second]->mWorth.toString();
            }
        }
    }
    return {resp_payload};
}

crow::response REST_App::HandleHistories(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (!finishedSolving) { return {400, "No MDP yet. Use /MDP to pass a problem file."}; }
    if (!json_req) { return {400, "Invalid JSON request"}; }
    auto policy_ids = json_req["policy_ids"].lo();

    json resp_payload = json::object();
    for (const auto& policy_id : policy_ids) {
        int policy_id_int = (int)policy_id.i();
        resp_payload[std::to_string(policy_id_int)] = json::array();
        for (auto &h : runner->histories[policy_id_int]) {
            resp_payload[std::to_string(policy_id_int)].push_back((JSONBuilder::toJSON(*h)));
        }
    }
    return {resp_payload.dump()};
}


crow::response REST_App::HandleSortSuccessors(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (!finishedSolving) { return {400, "No MDP yet. Use /MDP to pass a problem file."}; }
    if (!json_req) { return {400, "Invalid JSON request"}; }
    auto policy_idx = json_req["policy_idx"].i();
    size_t hist_idx = -1;
    vector<size_t>* path = nullptr;
    if (json_req.has("hist_idx")) {
        hist_idx = json_req["hist_idx"].i();
        path = runner->histories[policy_idx][hist_idx]->path.get();
    }
    auto con_idx = json_req["consideration_idx"].i();

    // Extract state successors from policy or history
    vector<Successor*> scrs;
    vector<WorthBase*> scr_worth;
    vector<bool> is_pos;
    vector<size_t> state_stack(1,0);
    auto null_w = runner->mdp->considerations[con_idx]->UniqueWorth();
    while (!state_stack.empty()) {
        size_t state_idx = state_stack.back();
        state_stack.pop_back();
        auto act_idx = runner->policies[policy_idx]->getAction(static_cast<int>(state_idx));
        if (!act_idx) {
            continue;
        }
        auto state_scrs = runner->mdp->getActionSuccessors(state_idx, act_idx.value());
        for (auto scr : *state_scrs) {
            if (path != nullptr && find(path->begin(), path->end(), scr->target) == path->end()) {
                continue;
            }
            scrs.push_back(scr);
            auto w = runner->mdp->considerations[con_idx]->judge(*scr);
            scr_worth.push_back(w);
            is_pos.push_back(w->compare(*null_w)==1);
            state_stack.push_back(scr->target);
        }
    }

    auto norm = runner->mdp->considerations[con_idx]->normalise(scr_worth);
    json resp_payload = json::object();
    resp_payload["Successors"] = json::object();
    resp_payload["Total_policy_successors"] = scrs.size();
    for (size_t i = 0; i < scrs.size(); i++) {
        auto scr = scrs[i];
        string src_str = std::to_string(scr->source);
        string tar_str = std::to_string(scr->target);
        if (!resp_payload["Successors"].contains(src_str)) {
            resp_payload["Successors"][src_str] = json::object();
        }
        auto scr_obj = json::object();
        scr_obj["norm"] = norm[i];
        scr_obj["is_pos"] = is_pos[i];
        resp_payload["Successors"][src_str][tar_str] = scr_obj;
    }
    // Probably better if it maps source state to list of targets, and each successor has a rank associated.
    // Then, don't have to scan all successors on client
    return {resp_payload.dump()};
}

crow::response REST_App::HandleGetNeccMEHR(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (!finishedSolving) { return {400, "No MDP yet. Use /MDP to pass a problem file."}; }
    if (!json_req) { return {400, "Invalid JSON request"}; }

    // Extract request
    size_t state_id;
    std::string action_label;
    size_t factPolicy_id;
    bool didGeneratePolicies = false;
    try {
        state_id = (int)json_req["state_id"].i();
        action_label = json_req["actionLabel"].s();
        factPolicy_id = (int)json_req["factPolicyIdx"].i();
    } catch (runtime_error &err) {
        return {400, format("Failed to read GetNeccMEHR request. {}", err.what())};
    }

    // Get the action index.
    auto action_idx = runner->mdp->getActionIndex(state_id, action_label);
    if (!action_idx) {
        return {400, format("No action '{}' on state {}", action_label, state_id)};
    }

    // Collect necessary foil policies
    vector<size_t> necc_policies(runner->policies.size(), 0);
    iota(necc_policies.begin(), necc_policies.end(), 0);
    std::erase_if(necc_policies, [this, state_id, action_idx](auto policy_idx) {
        return runner->policies[policy_idx]->policy[static_cast<int>(state_id)] != action_idx.value();
    });
    // If no policies, it will be equivalent to others. Plan now to generate them
    if (necc_policies.empty()) {
        didGeneratePolicies = true;
        auto er = runner->explain(state_id, action_idx.value(), runner->policies[factPolicy_id].get());
        necc_policies = er.newPolicyIndices;
        std::cout << runner->policies[necc_policies[0]]->toString() << std::endl;
    }
    auto it = std::min_element(necc_policies.begin(), necc_policies.end(),
        [this](const size_t& lhs, const size_t& rhs) {
        return runner->non_accept->getPolicyNonAccept(lhs) < runner->non_accept->getPolicyNonAccept(rhs);
    });
    double min_nacc = runner->non_accept->getPolicyNonAccept(*it.base());
    std::erase_if(necc_policies, [min_nacc, this](const size_t& pi_idx) {
        return runner->non_accept->getPolicyNonAccept(pi_idx) != min_nacc;
    });
    // Save the indices of the necessary foil policies.
    json foils = json::array();
    for (auto pi : necc_policies) {
        foils.push_back(pi);
    }
    // Add the fact policy, which is of course necessary.
    necc_policies.push_back(factPolicy_id);

    // Get attacks on necessary policies
    json hist_list = json::object();
    json attacks_on = json::object();

    vector<Attack> attacks;
    for (size_t pi_idx : necc_policies) {
        string pi_idx_str = std::to_string(pi_idx);
        // Get this policy's histories
        for (auto &h : runner->histories[pi_idx]) {
            hist_list[pi_idx_str].push_back((JSONBuilder::toJSON(*h)));
        }
        // Get attackers on this policy
        json attackers = json::object();
        for (auto &att : runner->mehr->attacks[pi_idx]) {

            attackers[to_string(att.sourcePolicyIdx)] = json::array();
            for (auto edge : att.HistoryEdges) {
                json att_obj = json::object();
                att_obj["src"] = edge.first;
                att_obj["tar"] = edge.second;
                att_obj["thy"] = att.theoryIdx;
                attackers[to_string(att.sourcePolicyIdx)].push_back(att_obj);
            }
        }
        attacks_on[pi_idx_str] = attackers;
    }


    // Aggregate response data and return
    json resp_payload = json::object();
    resp_payload[FIELD::HISTORIES] = hist_list;
    resp_payload[FIELD::ATTACKS] = attacks_on;
    resp_payload[FIELD::FOILSOLUTIONS] = foils;
    resp_payload[FIELD::SOLUTIONS] = json::array();
    if (didGeneratePolicies) {
        for (auto i : necc_policies) {
            if (i==factPolicy_id) {continue;}
            resp_payload[FIELD::SOLUTIONS].push_back(JSONBuilder::toJSON(*runner->policies[i], *runner->mdp, runner->non_accept->getPolicyNonAccept(i), true));
        }
    }
    return {resp_payload.dump()};
}

crow::response REST_App::HandlePlanFromHistory(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (!finishedSolving) { return {400, "No MDP yet. Use /MDP to pass a problem file."}; }
    if (!json_req) { return {400, "Invalid JSON request"}; }
    finishedSolving = false;// prevent other requests
    // Extract request
    size_t history_idx;
    size_t policy_idx;
    string file_in;
    size_t real_time;
    try {
        history_idx = (int)json_req["stateIdx"].i();
        policy_idx = (int)json_req["policyIdx"].i();
        real_time = (int)json_req["realTime"].i();
        file_in = json_req["file_in"].s();
    } catch (runtime_error &err) {
        return {400, format("MPlan failed to read PlanFromHistory request. {}", err.what())};
    }
    // Save history
    auto hist = std::move(runner->histories[policy_idx][history_idx]);
    // Read new mdp
    std::string inp = DATA_FOLDER_PATH;
    if (json_req.has("from_data_folder") && json_req["from_data_folder"].b()==true) {
        file_in = inp + file_in;
    }
    std::cout << "New file in: " << file_in << std::endl;
    runner = make_unique<Runner>(file_in);
    runner->make_history_paths = true;
    std::string of = OUTPUT_FOLDER_PATH;
    std::string file_out = format("{}ServerRequest_t{}.json", of, hist->path->size() + real_time);
    runner->WriteTo(file_out);
    json resp;
    resp["file_out"] = file_out;
    finishedSolving = true;
    return {resp.dump()};
}

crow::response REST_App::HandleCacheSuccessors(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (!finishedSolving) { return {400, "No MDP yet. Use /MDP to pass a problem file."}; }
    if (!json_req) { return {400, "Invalid JSON request"}; }
    finishedSolving = false;// prevent other requests

    // Extract request
    vector<int> actions_index;
    vector<size_t> states_index;
    try {
        for (auto &i : json_req["states_index"]) {
            states_index.push_back(i.i());
        }
        if (json_req["actions"].size() != states_index.size() - 1) {
            return {400,
                format("Invalid request. Number of actions {} and number of successor states {} mismatch.",
                    json_req["actions"].size(), states_index.size() - 1)};
        }
        for (size_t i = 0; i < json_req["actions"].size(); ++i) {
            string a = json_req["actions"][i].s();
            if (auto aIdx = runner->mdp->getActionIndex(states_index[i], a)) {
                actions_index.push_back(static_cast<int>(aIdx.value()));
            }
            else {
                return {400,
                format("Invalid request. No action {} for state {}",
                    a, states_index[i])};
            }
        }
    } catch (runtime_error &err) {
        return {400, format("MPlan failed to read PlanFromHistory request. {}", err.what())};
    }
    for (size_t i = 0; i < actions_index.size(); ++i) {
        size_t src_state = states_index[i];
        size_t tar_state = states_index[i+1];
        QValue qv = QValue(*runner->mdp);
        TotalHistory.emplace_back(*runner->mdp);
        auto scrs = runner->mdp->getActionSuccessors(src_state, actions_index[i]);
        auto scr = std::find_if(scrs->begin(), scrs->end(), [tar_state](Successor* scr) {return scr->target==tar_state;});
        runner->mdp->AggregateWithCertainSuccessor(TotalHistory.back(), *scr);
    }
    finishedSolving = true;
    return {};
}

crow::response REST_App::HandleAggregateCachedSuccessors(const crow::request &req) {
    auto json_req = crow::json::load(req.body);
    if (!finishedSolving) { return {400, "No MDP yet. Use /MDP to pass a problem file."}; }
    finishedSolving = false;// prevent other requests
    vector<QValue*> tempWorth;
    vector<QValue*> tempBaseline;
    vector<double> probs;
    tempBaseline.reserve(TotalHistory.size());
    for (auto & i : TotalHistory) {
        tempWorth.emplace_back(&i);
        tempBaseline.emplace_back(new QValue(*runner->mdp));
        probs.emplace_back(1);
    }
    // Aggregate response data, cleanup, and return.
    auto qv = runner->mdp->MultiGather(tempWorth, probs, tempBaseline);
    for (auto p : tempBaseline) {
        delete p;
    }
    TotalHistory.clear();
    finishedSolving = true;
    json resp_payload = json::object();
    resp_payload["Worth"] = qv.toStringVector();
    return {resp_payload.dump()};
}
