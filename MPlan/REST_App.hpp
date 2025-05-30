//
// Created by Simon Kolker on 08/04/2025.
//

#pragma once
#include "crow_all.h"
#include "Runner.hpp"
#include "Solver.hpp"


class REST_App {
    crow::App<crow::CORSHandler> app;
    bool finishedSolving = false;
    unique_ptr<Runner> runner;
public:

    REST_App() {
        runner = make_unique<Runner>();
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
            runner->solver->getCandidates(*s, candidates, indicesOfUndominated, qValueIdxToAction);

            crow::json::wvalue r;
            // Make an object + fields for each action
            for (int i=0; i<candidates.size(); i++) {
                std::string act = (runner->mdp->getActions(*s))->at(qValueIdxToAction[i])->label;
                r[act] = crow::json::wvalue(crow::json::type::Object);
                r[act]["containsUndominated"] = false;
                r[act]["QValues"] = crow::json::wvalue(crow::json::type::List);
            }
            for (int i=0; i<candidates.size(); i++) {
                std::string act = (runner->mdp->getActions(*s))->at(qValueIdxToAction[i])->label;
                // Add qvalue to action.
                r[act]["QValues"][r[act]["QValues"].size()] = crow::json::wvalue(crow::json::type::Object);
                r[act]["QValues"][r[act]["QValues"].size()-1]["Value"] = candidates[i].toStringVector();
                r[act]["QValues"][r[act]["QValues"].size()-1]["isUndominated"] = std::find(indicesOfUndominated.begin(), indicesOfUndominated.end(), i)!=indicesOfUndominated.end();
                r[act]["containsUndominated"] = r[act]["containsUndominated"].dump()=="true" || std::find(indicesOfUndominated.begin(), indicesOfUndominated.end(), i)!=indicesOfUndominated.end();
            }
            return crow::response(r);
        });


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
                runner->setInputFile(file_in);
                std::string of = OUTPUT_FOLDER_PATH;
                std::string file_out = of + "ServerRequest.json";
                runner->writeTo(file_out);
                crow::json::wvalue resp_payload;
                resp_payload["file_out"] = file_out;
                finishedSolving = true;
                return crow::response(resp_payload);
        }
        catch (runtime_error e) {
            std::cout << e.what() << "\n";
            finishedSolving = true;
            return crow::response(500);
        }
        });
        CROW_ROUTE(app, "/MEHR").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            std::cout << "MEHR REQUEST \n";
            // Process Request
            auto json_req = crow::json::load(req.body);
            if (!finishedSolving) {
                return crow::response(400, "No MDP yet. Use /MDP to pass a problem file.");
            }
            if (!json_req) {
                return crow::response(400, "Invalid JSON request");
            }
            if (!json_req.has("policy_ids")) {
                return crow::response(400, "Invalid QValue Request: No state_id int.");
            }
            auto policy_ids = json_req["policy_ids"].lo();


            crow::json::wvalue resp_payload;
            for (const auto& policy_id : policy_ids) {
                int policy_id_int = (int)policy_id.i();
                resp_payload[policy_id_int] = crow::json::wvalue(crow::json::type::List);
                for (Attack& att : (runner->mehr->attacksByTarget)->at(policy_id_int) ) {
                    ushort idx = resp_payload[policy_id_int].size();
                    resp_payload[policy_id_int][idx] = crow::json::wvalue(crow::json::type::Object);
                    resp_payload[policy_id_int][idx]["sourceHistory"] = att.sourceHistoryIdx;
                    resp_payload[policy_id_int][idx]["sourcePolicy"] = att.sourcePolicyIdx;
                    resp_payload[policy_id_int][idx]["targetHistory"] = att.targetHistoryIdx;
                    resp_payload[policy_id_int][idx]["probability"] = att.probability;
                    resp_payload[policy_id_int][idx]["sourceHistoryQValue"] = runner->histories[att.sourcePolicyIdx][att.sourceHistoryIdx]->worth.toString();
                    resp_payload[policy_id_int][idx]["targetHistoryQValue"] = runner->histories[att.targetPolicyIdx][att.targetHistoryIdx]->worth.toString();
                }
            }
            return crow::response(resp_payload);
        });

        app.port(18080)
        //.multithreaded()
        .run();

    }

};



