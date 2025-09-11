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

#include "Absolutism.hpp"
#include "Maximin.hpp"
#include "Utilitarianism.hpp"
#include "Threshold.hpp"
#include "MoralTheory.hpp"


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
    std::ostringstream oss;
    verifyJSON(data);

    // HORIZON
    horizon = data["horizon"];

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
    for (std::string act : data["actions"]) {
        actionMap[act] = make_unique<Action>(actionId, act);
        actionId++;
    }
}

void MDP::theoriesFromJSON(nlohmann::json &data) {
    std::vector<int> ranks = std::vector<int>();
    for (json t : data["theories"] ) {
        ranks.push_back(t["Rank"]);
    }
    // std sorts ranks uniquely
    std::set<int> unique_ordered_ranks(ranks.begin(), ranks.end());
    groupedTheoryIndices = std::vector<std::vector<size_t>>(unique_ordered_ranks.size());
    for (auto& i : groupedTheoryIndices) {
        i = vector<size_t>();
    }

    size_t conID = 0;
    for (json t : data["theories"] ) {
        std::string type = t["Type"];
        std::string name = t["Name"];
        MEHRTheory *m;
        if (type == "Utility") {
            this->considerations.push_back(new Utilitarianism(t, conID));
            AddMEHRTheory(new MEHRUtilitarianism(t["Rank"], conID, mehr_theories.size(), name), t["Rank"], unique_ordered_ranks);
        } else if (type=="Cost") {
            // Make Utiltarian theory
            considerations.push_back(new Utilitarianism(t, conID));
            budget = t["Budget"];
            non_moralTheoryIdx = static_cast<int>(conID);
        } else if (type=="Threshold") {
            this->considerations.push_back(new Threshold(t, conID));
            m = new MEHRThreshold(t["Rank"], conID, mehr_theories.size(), t["Threshold"], name);
            AddMEHRTheory(m, t["Rank"], unique_ordered_ranks);
        } else if (type=="Absolutism") {
            this->considerations.push_back(new Absolutism(t, conID));
            m = new MEHRAbsolutism(t["Rank"], mehr_theories.size(), conID, name);
            AddMEHRTheory(m, t["Rank"], unique_ordered_ranks);
        }
        else if (type=="PoDE") {
            // TO IMPLEMENT...
        } else if (type=="Maximin") {
            std::string mehr_name = t["Component_Of"];
            int mehr_id = findMEHRTheoryIdx(mehr_name);
            MEHRMaximin* mehrMaximin;
            if (mehr_id == -1) {
                // no theory exists -> make one.
                mehrMaximin = new MEHRMaximin(t["Rank"], mehr_theories.size(), mehr_name);
                AddMEHRTheory(mehrMaximin, t["Rank"], unique_ordered_ranks);
            } else {
                mehrMaximin = dynamic_cast<MEHRMaximin*>(mehr_theories[mehr_id]);
            }
            considerations.push_back(new Maximin(t, conID));
            mehrMaximin->addConsideration(conID);
        }
        else {
            std::ostringstream oss;
            oss << "MDP::buildFromJSON: unrecognized theory type, '" << type << "'";
            throw runtime_error(oss.str());
        }
        conID = considerations.size();
        // TODO Other theories...
    }
}

size_t MDP::AddMEHRTheory(MEHRTheory *mehrTheory, int rank, std::set<int> &unique_ordered_ranks) {
    size_t index = distance(unique_ordered_ranks.begin(), unique_ordered_ranks.find(rank));
    groupedTheoryIndices[index].push_back(mehr_theories.size());
    mehr_theories.push_back(mehrTheory);
    return index;
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
    total_states = data["total_states"];
    states = std::vector<State*>(total_states);
    stateActions = std::vector<std::vector<shared_ptr<Action>>>(total_states);
    State* state;
    auto s_t = data["state_transitions"];
    for (int i=0; i < this->total_states; ++i) {
        int number_of_actions = 0;
        if (s_t.is_array()) { number_of_actions = s_t[i].size(); }
        if (s_t.is_object()) { number_of_actions = s_t[to_string(i)].size(); }
        state = new State(i,number_of_actions, data["state_time"][i]);
        states[i] = state;
        stateActions[i] = vector<shared_ptr<Action>>();
    }
    for (int gIdx : data["goals"]) {
        this->states[gIdx]->isGoal = true;
    }
    // May not be state tags, so skip final step if not.
    if (!data.contains("state_tags")) {
        return;
    }
    if (data.count("state_tags")) {
        for (int i=0; i <this->total_states; ++i) {
            states[i]->tag = data["state_tags"][i];
        }
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
                for (int i = 0; i < this->considerations.size(); ++i) {
                    this->considerations[i]->processSuccessor(successor, successorData[i+2]);
                }

            }
        }
    }
}
