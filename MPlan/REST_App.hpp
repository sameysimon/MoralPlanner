//
// Created by Simon Kolker on 08/04/2025.
//

#pragma once
#include "crow_all.h"
#include <nlohmann/json.hpp>
#include "Runner.hpp"
#include "Solver.hpp"
#include "sstream"
#include "JSONBuilder.hpp"

using json = nlohmann::json;

class REST_App {
    crow::App<crow::CORSHandler> app;
    bool finishedSolving = false;
    unique_ptr<Runner> runner;
public:

    REST_App() {

        // Give a json file address.
        CROW_ROUTE(app, "/MDP").methods(crow::HTTPMethod::POST)([this](const crow::request &req){
            std::cout << "MDP REQUEST \n";
            finishedSolving = false;// prevent other requests
            try {
                auto json_req = crow::json::load(req.body);
                if (!json_req) {
                    return crow::response(400, "Invalid JSON request");
                }
                if (!json_req.has("file_in")) {
                    return crow::response(400, "Invalid MDP Request: No file_in path.");
                }
                std::string inp = DATA_FOLDER_PATH;
                std::string file_in = json_req["file_in"].s();
                if (!json_req["from_root"]) {
                    file_in = inp + file_in;
                }
                runner = make_unique<Runner>();
                runner->setInputFile(file_in);
                std::string of = OUTPUT_FOLDER_PATH;
                std::string file_out = of + "ServerRequest.json";
                runner->writeTo(file_out);
                json resp;
                resp["file_out"] = file_out;
                finishedSolving = true;
                return crow::response(resp.dump());
        }
        catch (runtime_error &e) {
            std::cout << e.what() << "\n";
            finishedSolving = true;
            return crow::response(500);
        }
        });

        // Request for the State-action for all actions at a given state.
        CROW_ROUTE(app, "/QValues").methods(crow::HTTPMethod::POST)([this](const crow::request &req){
            auto json_req = crow::json::load(req.body);
            if (!finishedSolving) {
                return crow::response(400, "No MDP yet. Use /MDP to pass a problem file.");
            }
            if (!json_req) {
                return crow::response(400, "Invalid JSON request");
            }
            if (!json_req.has("state_id")) {
                return crow::response(400, "Invalid QValue Request: No state_id int.");
            }
            int state_id = (int)json_req["state_id"].i();

            // Values we need
            vector<QValue> candidates = vector<QValue>();// The QValue candidates (corresponding to state-actions)
            vector<int> indicesOfUndominated = vector<int>();// Indices of candidate QValues that are undominated.
            vector<int> qValueIdxToAction = vector<int>();// Maps QValue index to action index.
            // Get the values
            State* s = runner->mdp->states[state_id];
            runner->solver->getUnDomCandidates(*s, candidates, indicesOfUndominated, qValueIdxToAction);

            json r;
            return crow::response(JSONBuilder::toJSON(candidates, indicesOfUndominated, qValueIdxToAction, *runner->mdp, *s).dump());

        });

        CROW_ROUTE(app, "/GetPolicyAttacks").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            auto json_req = crow::json::load(req.body);
            if (!finishedSolving) { return crow::response(400, "No MDP yet. Use /MDP to pass a problem file."); }
            if (!json_req) { return crow::response(400, "Invalid JSON request"); }
            size_t policyIdx;
            try {
                policyIdx = (int)json_req["policyIdx"].i();
            } catch (runtime_error &err) {
                std::stringstream ss;
                ss << err.what();
                ss << "Failed to read request." << std::endl;
                std::cout << ss.str() << "\n";
                return crow::response(400, ss.str());
            }
            if (policyIdx< runner->mehr->attacks.size()) {
                return crow::response(400, std::format("Invalid request for MEHR attacks. Policy with index {} does not exist.", policyIdx));
            }
            json resp = JSONBuilder::toJSON(runner->mehr->attacks[policyIdx]);
            return crow::response(resp.dump());
        });

        // Request an explanation
        CROW_ROUTE(app, "/Explain").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            auto json_req = crow::json::load(req.body);
            if (!finishedSolving) { return crow::response(400, "No MDP yet. Use /MDP to pass a problem file."); }
            if (!json_req) { return crow::response(400, "Invalid JSON request"); }

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
                return crow::response(400, ss.str());
            }
            auto actions = runner->mdp->getActions(*runner->mdp->states[state_id]);
            size_t actionIdx;
            for (actionIdx=0; actionIdx < actions->size(); ++actionIdx) {
                if (actions->at(actionIdx)->label == action_label) {
                    break;
                }
            }
            if (actionIdx == actions->size()) {
                std::stringstream ss;
                ss << "Could not find action with label " << action_label << " for state with id " << state_id;
                std::cout << ss.str() << "\n";
                return crow::response(400, ss.str());
            }
            explainResult er = runner->explain(state_id, actionIdx, runner->policies[factPolicy_id]);

            json resp = JSONBuilder::toJSON(er, *runner);
            auto x = resp.dump();
            return crow::response(x);
        });

        // Get Attacks on given policies
        CROW_ROUTE(app, "/MEHR").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            std::cout << "MEHR REQUEST \n";
            auto json_req = crow::json::load(req.body);
            if (!finishedSolving) { return crow::response(400, "No MDP yet. Use /MDP to pass a problem file."); }
            if (!json_req) { return crow::response(400, "Invalid JSON request"); }

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
                        resp_payload[policy_id_int][idx]["sourceHistoryQValue"] = runner->histories[att.sourcePolicyIdx][edge.first]->worth.toString();
                        resp_payload[policy_id_int][idx]["targetHistoryQValue"] = runner->histories[att.targetPolicyIdx][edge.second]->worth.toString();
                    }
                }
            }
            return crow::response(resp_payload);
        });

        CROW_ROUTE(app, "/Histories").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            auto json_req = crow::json::load(req.body);
            if (!finishedSolving) { return crow::response(400, "No MDP yet. Use /MDP to pass a problem file."); }
            if (!json_req) { return crow::response(400, "Invalid JSON request"); }
            auto policy_ids = json_req["policy_ids"].lo();

            json resp_payload = json::object();
            for (const auto& policy_id : policy_ids) {
                int policy_id_int = (int)policy_id.i();
                resp_payload[std::to_string(policy_id_int)] = json::array();
                for (auto h : runner->histories[policy_id_int]) {
                    resp_payload[std::to_string(policy_id_int)].push_back((JSONBuilder::toJSON(*h)));
                }
            }
            return crow::response(resp_payload.dump());
        });

        app.port(18080)
        //.multithreaded()
        .run();

    }

};



