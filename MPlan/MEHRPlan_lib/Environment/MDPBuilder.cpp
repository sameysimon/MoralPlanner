//
// Created by e56834sk on 21/08/2024.
//
#include "MDP.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <set>

#include "Absolutism.hpp"
#include "Maximin.hpp"
#include "MEHR.hpp"
#include "Utilitarianism.hpp"
//#include "OrdinalCase.hpp"
//#include "Threshold.hpp"
#include "MoralTheory.hpp"
#include "OrdinalCase.hpp"


using json = nlohmann::json;
using namespace std;

MDP::MDP(nlohmann::json& data) {
    buildFromJSON(data);
}

MDP::MDP(const string& fileName) {
    std::ifstream file(fileName);
    std::cout << fileName << " opened successfully." << std::endl;
    json data;
    try {
        data  = json::parse(file);
        file.close();
    }
    catch (json::parse_error& ex) {
        std::cout << "Cannot read file: " << ex.what() << std::endl;
        throw ex;
    }
    this->buildFromJSON(data);
}

bool isJSONNumeric(nlohmann::json& data) {

    return data.type() == nlohmann::detail::value_t::number_integer || data.type() == nlohmann::detail::value_t::number_unsigned || data.type() == nlohmann::detail::value_t::number_float;
}
void throwStageOne(string msg) {
    throw std::runtime_error("MDP::buildFromJSON: " + msg);
}
void verifyJSON(nlohmann::json& data) {
    if (data.contains("Horizon") == false) {
        throwStageOne("horizon not found.");
    }
    if (data.contains("Actions") == false) {
        throwStageOne("actions not found.");
    }
    if (data.contains("Total_states") == false) {
        throwStageOne("total_states not found.");
    }
    if (data.contains("Theories") == false) {
        throwStageOne("theories not found.");
    }
    if (data.contains("State_transitions") == false) {
        throwStageOne("state_transitions not found.");
    }
    for (auto theory : data["Theories"]) {
        if (theory.contains("Name") == false) {
            throwStageOne("theory name not found.");
        }
        string theoryName = theory["Name"];
        if (theory.contains("Rank") == false) {
            throwStageOne(theoryName + " Rank not found.");
        }
        if (theory.contains("Type") == false) {
            throwStageOne(theoryName + " Type not found.");
        }
    }

    for (auto &con : data["Considerations"]) {
        if (con.contains("Name") == false) {
            throwStageOne("A consideration has no name.");
        }
        string conName = con["Name"];
        if (con.contains("Type") == false) {
            throwStageOne(conName + " Type not found.");
        }
        if (con.contains("Component_of") == false && con["Type"]!="Cost") {
            throwStageOne(conName + " is not non-moral, but is not the component of any theory.");
        }
    }



    int theories = data["Considerations"].size();
    int stateID = 0;
    for (auto stateTransitions : data["State_transitions"]) {
        for (auto &transition : stateTransitions.items()) {
            string key = transition.key();
            vector<string> actions = data["Actions"];
            if (std::find(actions.begin(), actions.end(), key) == actions.end()) {
                throw std::runtime_error("MDP::buildFromJSON: under state " + std::to_string(stateID) + " action " + key + " not found.");
            }
            int successor_count=0;
            for (auto successor : transition.value()) {
                if (successor.size() < theories + 2) {
                    throw std::runtime_error("MDP::buildFromJSON: under state " + std::to_string(stateID) + ", action " + key + " successor " + std::to_string(successor_count) + " not enough theory data.");
                }
                if (not (isJSONNumeric(successor[0]) and isJSONNumeric(successor[1]))) {
                    throw std::runtime_error("MDP::buildFromJSON: under state " + std::to_string(stateID) + ", action " + key + " successor " + std::to_string(successor_count) + "probability, or transition not numeric.");
                }

                if (successor[0] > 1) {
                    throw std::runtime_error("MDP::buildFromJSON: under state " + std::to_string(stateID) + ", action " + key + " successor " + std::to_string(successor_count) + "probability greater than 1.");
                }
                if (successor[1] >= data["Total_states"] or successor[1] < 0) {
                    throw std::runtime_error("MDP::buildFromJSON: under state " + std::to_string(stateID) + ", action " + key + " successor " + std::to_string(successor_count) + "successor index not in state range.");
                }
                successor_count++;
            }


        }
        stateID++;
    }
    if (stateID != data["Total_states"]) {
        int totStates = data["Total_states"];
        throw std::runtime_error("MDP::buildFromJSON: There were " + std::to_string(stateID) + " state transition maps, but total_states is " + std::to_string(totStates));
    }
}

void MDP::buildFromJSON(nlohmann::json& data) {
    std::ostringstream oss;
    verifyJSON(data);

    // HORIZON
    horizon = data["Horizon"];

    try {
        actionsFromJSON(data);
    } catch (nlohmann::json::exception& e) {
        cerr << "MDP::buildFromJSON. Failed to load action data: " << e.what() << endl;
        exit(1);
    }

    try {
        theoriesFromJSON(data);
    } catch (nlohmann::json::exception& e) {
        cerr << "MDP::buildFromJSON. Bad JSON for Moral Theories. " << e.what() << endl;
        exit(1);
    }

    try {
        statesFromJSON(data);
    } catch (nlohmann::json::exception& e) {
        oss << "MDP::buildFromJSON. Failed to load states data. " << e.what();
        throw std::runtime_error(oss.str());
    }

    try {
        successorsFromJSON(data);
    } catch (nlohmann::json::exception& e) {
        cerr << "MDP::buildFromJSON. Failed to load successor data: " << e.what() << endl;
        exit(1);
    }

}


void MDP::actionsFromJSON(nlohmann::json& data) {
    int actionId = 0;
    for (std::string act : data["Actions"]) {
        actionMap[act] = make_unique<Action>(actionId, act);
        actionId++;
    }
}

void MDP::theoriesFromJSON(nlohmann::json &data) {
    std::vector<int> ranks = std::vector<int>();
    for (json t : data["Theories"] ) {
        ranks.push_back(t["Rank"]);
    }
    // std sorts ranks uniquely
    std::set<int> unique_ordered_ranks(ranks.begin(), ranks.end());
    groupedTheoryIndices = std::vector<std::vector<size_t>>(unique_ordered_ranks.size());

    for (json &t : data["Theories"]) {
        std::string name = t["Name"];
        std::string type = t["Type"];
        int rank = t["Rank"];
        MEHRTheory *theory;

        if (type == "Utility") {
            theory = new MEHRUtilitarianism(t["Rank"], mehr_theories.size(), name);
        }
        else if (type == "Threshold") {
            //theory = new MEHRThreshold(t["Rank"], mehr_theories.size(), t["Threshold"], name);
        }
        else if (type == "Absolutism") {
            theory = new MEHRAbsolutism(t["Rank"], mehr_theories.size(), name);
        }
        else if (type == "Maximin") {
            theory = new MEHRMaximin(t["Rank"], mehr_theories.size(), name);
        }
        else if (type == "Ordinal") {
            theory = new MEHROrdinal(t["Rank"], mehr_theories.size(), name);
        }
        else {
            std::cout << "MDP::theoriesFromJSON - Theory type " << type << " does not exist." << std::endl;
            throw std::runtime_error(std::format(
                "MDP::buildFromJSON: Theory {} has type '{}' that does not exist.",
                name, type
            ));
        }
        size_t index = distance(unique_ordered_ranks.begin(), unique_ordered_ranks.find(rank));
        groupedTheoryIndices[index].push_back(mehr_theories.size());
        mehr_theories.push_back(theory);
    }

    size_t conID = 0;
    for (json &t : data["Considerations"]) {
        std::string type = t["Type"];
        std::string name = t["Name"];

        std::vector<std::string> mehr_names;
        if (t["Component_of"].type() == nlohmann::json::value_t::string) {
            // Empty string implies attachment to no moral theory.
            if (t["Component_of"] != "") {
                mehr_names.push_back(t["Component_of"]);
            }
        }else {
            mehr_names = t["Component_of"].get<std::vector<std::string>>();
        }

        if (type == "Utility") {
            if (t.contains("type") && t["Utility_type"] == "Maximum") {
                this->considerations.push_back(new MaxiUtilitarianism(t, conID));
            } else if (t.contains("type") && t["Utility_type"] == "Minimum") {
                this->considerations.push_back(new MiniUtilitarianism(t, conID));
            } else {
                this->considerations.push_back(new Utilitarianism(t, conID));
            }
        }
        else if (type == "Cost") {
            this->considerations.push_back(new Utilitarianism(t, conID));
            budget = t["Budget"];
            non_moralTheoryIdx = static_cast<int>(conID);
        }
        else if (type == "Absolutism") {
            this->considerations.push_back(new Absolutism(t, conID));
        }
        else if (type == "Ordinal") {
            this->considerations.push_back(new Ordinal(t, conID));
        }
        else {
            throw std::runtime_error(std::format(
                "MDP::buildFromJSON: Consideration {} has type '{}' that does not exist.",
                name, type
            ));
        }

        // Add consideration to all relevant theories.
        for (std::string &mehrName : mehr_names) {
            int mehrIdx = findMEHRTheoryIdx(mehrName);
            if (mehrIdx < 0) {
                throw runtime_error(std::format("MDP::buildFromJSON: Consideration '{}' references theory with name '{}' that has not been declared.", name, mehrName));
            }
            mehr_theories[mehrIdx]->AddConsideration(*this->considerations[conID]);
        }
        conID = considerations.size();
    }

}

int MDP::findMEHRTheoryIdx(string& theoryName) {
    for (int i = 0; i < mehr_theories.size(); i++) {
        if (mehr_theories[i]->mName == theoryName) {
            return i;
        }
    }
    return -1;
}


void MDP::statesFromJSON(nlohmann::json &data) {
    total_states = data["Total_states"];
    states = std::vector<State*>(total_states);
    stateActions = std::vector<std::vector<shared_ptr<Action>>>(total_states);
    State* state;
    auto s_t = data["State_transitions"];
    for (int i=0; i < this->total_states; ++i) {
        int number_of_actions = 0;
        if (s_t.is_array()) { number_of_actions = s_t[i].size(); }
        if (s_t.is_object()) { number_of_actions = s_t[to_string(i)].size(); }
        state = new State(i,number_of_actions, data["State_time"][i]);
        states[i] = state;
        stateActions[i] = vector<shared_ptr<Action>>();
    }
    if (data.contains("Goals")) {
        for (int gIdx : data["Goals"]) {
            this->states[gIdx]->isGoal = true;
        }
    }
    // May not be state tags, so skip final step if not.
    if (!data.contains("State_tags")) {
        return;
    }
    if (data.count("State_tags")) {
        for (int i=0; i <this->total_states; ++i) {
            states[i]->tag = data["State_tags"][i];
        }
    }



}

void MDP::successorsFromJSON(nlohmann::json &data) {
    auto t = data["State_transitions"];
    Successor* successor;
    for (int sourceIdx=0; sourceIdx <total_states; ++sourceIdx) {
        json transitionDict;
        State* currState = states[sourceIdx];
        if (t.is_array()) {
            transitionDict = t[sourceIdx];
        } else if (t.is_object()) {
            transitionDict = t[std::to_string(sourceIdx)];
        }
        for (auto& actionSuccessors : transitionDict.items())
        {
            // get the action
            const std::string& actionLabel = actionSuccessors.key();
            auto action = this->actionMap[actionLabel];
            // Add action to list for this state.
            stateActions[currState->id].push_back(action);

            // create holder for action successors
            auto* successorSet = currState->addAction(actionLabel);
            json successorObjects = actionSuccessors.value();

            for (auto& successorData : successorObjects) {
                // Create successor
                double prob = successorData[0];
                int targetID = successorData[1];
                successor = new Successor(sourceIdx, targetID, prob);
                successorSet->push_back(successor);

                // Pass successor judgement to moral theory
                for (int i = 0; i < this->considerations.size(); ++i) {
                    this->considerations[i]->processSuccessor(successor, successorData[i+2]);
                }

            }
        }
    }
}
