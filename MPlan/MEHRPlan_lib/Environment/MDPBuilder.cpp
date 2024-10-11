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
#include "Solution.hpp"


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
    json data  = json::parse(file);
    file.close();
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
                if (successor[1] > data["total_states"] or successor[1] < 0) {
                    throw std::runtime_error("MDP::buildFromJSON: under state " + std::to_string(stateID) + ", action " + key + " successor " + std::to_string(successor_count) + "successor index not in state range.");
                }
                successor_count++;
            }


        }
        stateID++;
    }



}

void MDP::buildFromJSON(nlohmann::json& data) {
    verifyJSON(data);
    // HORIZON
    horizon = data["horizon"];
    // ACTIONS
    int actionId = 0;
    actions = vector<Action*>(data["actions"].size());
    Action* a;
    for (std::string act : data["actions"]) {
        string* actLabel = new string(act);
        a = new Action(actionId, actLabel);
        actions[actionId] = a;
        actionMap[act] = a;
        actionId++;
    }


    // THEORIES
    // Create moral theories for first solution
    // Terrible stuff below. Find and order ranks, make
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
        if (type == "Utility") {
            // Make Utiltarian theory
            Utilitarianism* u = new Utilitarianism(theoryId);
            u->label = t["Name"];
            u->rank = t["Rank"];
            u->processHeuristics(t["Heuristic"]);
            int index = distance(unique_ordered_ranks.begin(), unique_ordered_ranks.find(t["Rank"]));
            groupedTheoryIndices[index].push_back(theories.size());
            this->theories.push_back(u);
        } else if (type=="Cost" and budget>-1) {
            throw runtime_error("MDP::buildFromJSON: Cannot have two amoral theories.");
        }
        else if (type=="Cost") {
            // Make Utiltarian theory
            Utilitarianism* u = new Utilitarianism(theoryId);
            u->label = t["Name"];
            budget = t["Budget"];
            u->processHeuristics(t["Heuristic"]);
            non_moralTheoryIdx = this->theories.size();
            this->theories.push_back(u);

            // Does not have a rank. Left out of rankings.
        } else if (type=="Threshold") {
            double long th = t["Threshold"];
            Threshold* u = new Threshold(theoryId, th);
            u->label = t["Name"];
            u->rank = t["Rank"];
            u->processHeuristics(t["Heuristic"]);
            int index = distance(unique_ordered_ranks.begin(), unique_ordered_ranks.find(t["Rank"]));
            groupedTheoryIndices[index].push_back(theories.size());
            this->theories.push_back(u);
        } else if (type=="Absolutism") {
            Absolutism* a = new Absolutism(theoryId);
            a->label = t["Name"];
            a->rank = t["Rank"];
            a->processHeuristics(t["Heuristic"]);
            int index = distance(unique_ordered_ranks.begin(), unique_ordered_ranks.find(t["Rank"]));
            groupedTheoryIndices[index].push_back(theories.size());
            this->theories.push_back(a);
        } else {
            throw runtime_error("MDP::buildFromJSON: unrecognized theory type.");
        }
        theoryId++;
        // TODO Other theories...
    }

    // STATES
    total_states = data["total_states"];
    states = std::vector<State*>(total_states);
    stateActions = std::vector<std::vector<Action*>>(total_states);
    State* state;
    auto s_t = data["state_transitions"];
    for (int i=0; i <this->total_states; ++i) {
        int number_of_actions = s_t[i].size();
        state = new State(i,number_of_actions);
        states[i] = state;
        stateActions[i] = vector<Action*>();

    }
    // GOALS
    // Set isGoal=true for goal states.
    for (int gIdx : data["goals"]) {
        this->states[gIdx]->isGoal = true;
    }


    // SUCCESSORS
    // Add successors to each state object
    auto t = data["state_transitions"];
    Successor* successor;
    for (int sourceIdx=0; sourceIdx <total_states; ++sourceIdx) {
        State* currState = states[sourceIdx];
        int stateActionIndex = 0;
        for (auto& actionSuccessors : t[sourceIdx].items())
        {
            // get the action
            std::string actionLabel = actionSuccessors.key();
            Action* action = this->actionMap[actionLabel];
            // Add action to list for this state.
            stateActions[currState->id].push_back(action);

            // create holder for action successors
            std::vector<Successor*>* successorSet = new std::vector<Successor*>();
            currState->actionSuccessors[stateActionIndex] = successorSet;
            // Create successor and populate
            json successorObjects = actionSuccessors.value();
            for (auto& successorData : successorObjects) {
                double long prob = successorData[0];
                int targetID = successorData[1];
                successor = new Successor(sourceIdx, targetID, prob);
                successorSet->push_back(successor);

                for (int i = 0; i < this->theories.size(); ++i) {
                    this->theories[i]->processSuccessor(successor, successorData[i+2]);
                }
            }
            stateActionIndex++;
        }
    }
}
