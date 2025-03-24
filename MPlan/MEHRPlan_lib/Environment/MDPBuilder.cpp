//
// Created by e56834sk on 21/08/2024.
//
#include "MDP.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <set>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "Absolutism.hpp"
#include "Utilitarianism.hpp"
#include "Threshold.hpp"
#include "MoralTheory.hpp"


using json = nlohmann::json;
using namespace std;

MDP::MDP(nlohmann::json& data) {
    buildFromJSON(data);
}

MDP::MDP(const string& fileName) {
    std::ifstream file(fileName); // Don't know how/if to put on heap?
    if (!file.is_open()) {
        throw std::runtime_error("File does not exist: '" + fileName + "'");
    }
    std::cout << fileName << " opened successfully." << std::endl;
    json data;
    try {
        data  = json::parse(file);
        file.close();
    }
    catch (json::parse_error& ex) {
        std::cerr << ex.what() << "at byte " << ex.byte << std::endl;
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
    if (data.contains("horizon") == false) {
        throwStageOne("horizon not found.");
    }
    if (data.contains("actions") == false) {
        throwStageOne("actions not found.");
    }
    if (data.contains("total_states") == false) {
        throwStageOne("total_states not found.");
    }
    if (data.contains("goals") == false) {
        throwStageOne("goals not found.");
    }
    if (data.contains("theories") == false) {
        throwStageOne("theories not found.");
    }
    if (data.contains("state_transitions") == false) {
        throwStageOne("state_transitions not found.");
    }
    for (auto theory : data["theories"]) {
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
        if (theory.contains("Heuristic") == false) {
            throwStageOne(theoryName + " Heuristic not found.");
        }
    }


    int theories = data["theories"].size();
    int stateID = 0;
    for (auto stateTransitions : data["state_transitions"]) {
        for (auto transition : stateTransitions.items()) {
            string key = transition.key();
            vector<string> actions = data["actions"];
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
                if (successor[1] >= data["total_states"] or successor[1] < 0) {
                    throw std::runtime_error("MDP::buildFromJSON: under state " + std::to_string(stateID) + ", action " + key + " successor " + std::to_string(successor_count) + "successor index not in state range.");
                }
                successor_count++;
            }


        }
        stateID++;
    }
    if (stateID != data["total_states"]) {
        int totStates = data["total_states"];
        throw std::runtime_error("MDP::buildFromJSON: There were " + std::to_string(stateID) + " state transition maps, but total_states is " + std::to_string(totStates));
    }


}



void MDP::buildFromJSON(nlohmann::json& data) {
    verifyJSON(data);
    // HORIZON
    horizon = data["horizon"];
    actionsFromJSON(data);

    theoriesFromJSON(data);

    statesFromJSON(data);

    successorsFromJSON(data);
}



void MDP::actionsFromJSON(nlohmann::json& data) {
    int actionId = 0;
    for (std::string act : data["actions"]) {
        actionMap[act] = make_unique<Action>(actionId, act);
        actionId++;
    }
}

void MDP::theoriesFromJSON(nlohmann::json &data) {
    std::vector<int> ranks = std::vector<int>();
    for (json t : data["theories"] )
        ranks.push_back(t["Rank"]);
    std::set<int> unique_ordered_ranks(ranks.begin(), ranks.end());
    groupedTheoryIndices = std::vector<std::vector<int>>(unique_ordered_ranks.size());
    for (auto& i : groupedTheoryIndices) {
        i = vector<int>();
    }
    int theoryId = 0;
    for (json t : data["theories"] ) {
        std::string type = t["Type"];
        MoralTheory *m;
        if (type == "Utility") {
            m = new Utilitarianism(t, theoryId);
        } else if (type=="Cost") {
            // Make Utiltarian theory
            m = new Utilitarianism(t, theoryId);
            budget = t["Budget"];
            non_moralTheoryIdx = this->theories.size();
        } else if (type=="Threshold") {
            m = new Threshold(t, theoryId);
        } else if (type=="Absolutism") {
            m = new Absolutism(t, theoryId);
        } else {
            throw runtime_error("MDP::buildFromJSON: unrecognized theory type.");
        }
        int index = distance(unique_ordered_ranks.begin(), unique_ordered_ranks.find(t["Rank"]));
        groupedTheoryIndices[index].push_back(static_cast<int>(theories.size()));
        this->theories.push_back(m);
        theoryId++;
        // TODO Other theories...
    }
}
void MDP::statesFromJSON(nlohmann::json &data) {
    total_states = data["total_states"];
    states = std::vector<State*>(total_states);
    stateActions = std::vector<std::vector<shared_ptr<Action>>>(total_states);
    State* state;
    auto s_t = data["state_transitions"];
    for (int i=0; i <this->total_states; ++i) {
        int number_of_actions = 0;
        if (s_t.is_array()) { number_of_actions = s_t[i].size(); }
        if (s_t.is_object()) { number_of_actions = s_t[to_string(i)].size(); }
        state = new State(i,number_of_actions, data["state_time"][i]);
        states[i] = state;
        stateActions[i] = vector<shared_ptr<Action>>();
    }
    if (data.count("state_tags")) {
        for (int i=0; i <this->total_states; ++i) {
            states[i]->tag = data["state_tags"][i];
        }
    }
    for (int gIdx : data["goals"]) {
        this->states[gIdx]->isGoal = true;
    }
}
void MDP::successorsFromJSON(nlohmann::json &data) {
    auto t = data["state_transitions"];
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
                for (int i = 0; i < this->theories.size(); ++i) {
                    this->theories[i]->processSuccessor(successor, successorData[i+2]);
                }

            }
        }
    }
}
