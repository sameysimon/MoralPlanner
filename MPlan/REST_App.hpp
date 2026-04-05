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
    ushort port = 18080;
    bool finishedSolving = false;
    unique_ptr<Runner> runner;
    vector<QValue> TotalHistory;
    optional<crow::response> BasicCheckValidRequest(crow::json::rvalue& json_req) const {
        if (!finishedSolving) {
            return crow::response(400, "No MDP yet. Use /MDP to pass a problem file.");
        }
        if (!json_req) {
            return crow::response(400, "Invalid JSON request");
        }
        return nullopt;
    }

public:
    crow::response HandleMDP(const crow::request &req);
    crow::response HandleQueryFoilAction(const crow::request &req);
    crow::response HandleQValues(const crow::request &req);
    crow::response HandleGetPolicyAttacks(const crow::request &req);
    crow::response HandlePlanLocked(const crow::request &req);
    crow::response HandleExplain(const crow::request &req);
    crow::response HandleMEHR(const crow::request &req);
    crow::response HandleHistories(const crow::request& req);
    crow::response HandleSortSuccessors(const crow::request& req);
    crow::response HandleGetNeccMEHR(const crow::request& req);
    crow::response HandlePlanFromHistory(const crow::request& req);

    crow::response HandleCacheSuccessors(const crow::request& req);
    crow::response HandleAggregateCachedSuccessors(const crow::request& req);


    REST_App(int port_ = 18080) {
        port = port_;
        //app.loglevel(static_cast<crow::LogLevel>(4 - LogLevel));
        InitHandlers();
    }


    void InitHandlers() {
        // Give a json file address.
        CROW_ROUTE(app, "/MDP").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
                return this->HandleMDP(req);
            });
        CROW_ROUTE(app, "/QueryFoilAction").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
            return this->HandleQueryFoilAction(req);
        });

        CROW_ROUTE(app, "/QValues").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
            return this->HandleQValues(req);
        });

        CROW_ROUTE(app, "/GetPolicyAttacks").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
            return this->HandleGetPolicyAttacks(req);
        });

        CROW_ROUTE(app, "/PlanLocked").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandlePlanLocked(req);
        });

        // Request an explanation
        CROW_ROUTE(app, "/Explain").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandleExplain(req);
        });

        // Get Attacks on given policies
        CROW_ROUTE(app, "/MEHR").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandleMEHR(req);
        });

        CROW_ROUTE(app, "/Histories").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandleHistories(req);
        });

        CROW_ROUTE(app, "/SortSuccessors").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandleSortSuccessors(req);
        });

        CROW_ROUTE(app, "/GetNeccMEHR").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandleGetNeccMEHR(req);
        });

        CROW_ROUTE(app, "/PlanFromHistory").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandlePlanFromHistory(req);
        });

        CROW_ROUTE(app, "/CacheSuccessors").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandleCacheSuccessors(req);
        });
        CROW_ROUTE(app, "/AggregateCachedSuccessors").methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
            return HandleAggregateCachedSuccessors(req);
        });


        app.port(port)
        //.multithreaded()
        .run();

    }





};



