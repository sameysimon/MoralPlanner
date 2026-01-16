//
// Created by Simon Kolker on 08/04/2025.
//

#include "REST_App.hpp"

crow::response REST_App::HandleMDP(const crow::request &req) {
    std::cout << "MDP REQUEST \n";
    finishedSolving = false;// prevent other requests
    try {
        auto json_req = crow::json::load(req.body);
        if (!json_req) {
            return {400, "Invalid JSON request"};
        }
        if (!json_req.has("file_in")) {
            return {400, "Invalid MDP Request: No file_in path."};
        }
        std::string inp = DATA_FOLDER_PATH;
        std::string file_in = json_req["file_in"].s();
        if (json_req.has("from_data_folder") && json_req["from_data_folder"].b()==true) {
            file_in = inp + file_in;
        }
        std::cout << file_in << std::endl;
        runner = make_unique<Runner>(file_in);
        std::string of = OUTPUT_FOLDER_PATH;
        std::string file_out = of + "ServerRequest.json";
        runner->WriteTo(file_out);
        json resp;
        resp["file_out"] = file_out;
        finishedSolving = true;
        return {resp.dump()};
    }
    catch (runtime_error &e) {
        std::cout << e.what() << "\n";
        finishedSolving = true;
        return crow::response(500);
    }
}