//
//  MDP.hpp
//  MPlan
//
//  Created by e56834sk on 10/07/2024.
//

#pragma once
#include <vector>
#include <list>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Action.hpp"
#include "State.hpp"
#include "Successor.hpp"
#include "MoralTheory.hpp"
#include <format>
#include <optional>

class QValue;
class Policy;
class MDP {
    // Maps action labels to action object
    std::unordered_map<std::string, std::shared_ptr<Action>> actionMap;
    // Vector of actions available for each state
    std::vector<std::vector<std::shared_ptr<Action>>> stateActions;
    void buildFromJSON(nlohmann::json& data);
    void actionsFromJSON(nlohmann::json& data);
    void theoriesFromJSON(nlohmann::json& data);
    int findMEHRTheoryIdx(std::string &theoryName);
    void statesFromJSON(nlohmann::json& data);
    void successorsFromJSON(nlohmann::json& data);

    int compareQValueByRank(QValue& qv1, QValue& qv2, int rank);
public:
    std::vector<Action*> actions;
    std::vector<State*> states;
    std::vector<Consideration*> considerations;
    std::vector<MEHRTheory*> mehr_theories;
    // Groups of MEHR moral theory indices, first holds lowest (best) ranked theories.
    std::vector<std::vector<size_t>> groupedTheoryIndices;

    //
    // Fields
    //
    int total_states=0;
    int horizon=3;
    float budget = -1;// initial budget is infinite
    int non_moralTheoryIdx=-1;

    //
    // Constructors.
    //
    explicit MDP(const std::string& fileName);
    explicit MDP(nlohmann::json& data);
    ~MDP();

    std::vector<Successor*>* getActionSuccessors(const size_t &state_idx, const int stateActionIndex) {
        if (state_idx> states.size()) {
            throw std::runtime_error(std::format("getActionSuccessors called with state {} which does not exist.", state_idx));
        }
        return states[state_idx]->actionSuccessors[stateActionIndex];
    }


    static std::vector<Successor*>* getActionSuccessors(const State &state, const int stateActionIndex) {
        if (state.actionSuccessors.empty()) {
            throw std::runtime_error(std::format("getActionSuccessors called with state {} and state-action Index {} has no successors.", state.id, stateActionIndex));
        }
         return state.actionSuccessors[stateActionIndex];
    }

    // Finds state-action index of action with matching label then returns pointer to successors.
    std::vector<Successor*>* getActionSuccessors(const State& state, const Action& action) {
        int i = 0;
        for (const auto& a : *getActions(state)) {
            if (a->label == action.label) {
                return getActionSuccessors(state, i);
            }
            i++;
        }
        throw std::runtime_error(std::format("MDP::getActionSuccessors State with id {} has no action with label {}", state.id, action.label));
    }
    std::vector<std::shared_ptr<Action>>* getActions(const State& state) {
        return &(stateActions[state.id]);
    }
    std::optional<std::string> getActionLabel(const size_t& state_idx, const std::size_t& action_idx) {
        if (state_idx > states.size()) { return std::nullopt; }
        auto actions = getActions(*states[state_idx]);
        if (action_idx > actions->size()) { return std::nullopt; }
        return actions->at(action_idx)->label;
    }
    std::optional<size_t> getActionIndex(const size_t& state_idx, const std::string& action_label) {
        return getActionIndex(*states[state_idx], action_label);
    }
    std::optional<size_t> getActionIndex(const size_t& state_idx, const Action& action) {
        return getActionIndex(*states[state_idx], action.label);
    }
    std::optional<size_t> getActionIndex(const State& state, const std::string& action_label) {
        auto actions = getActions(state);
        size_t action_idx;
        for (action_idx = 0; action_idx < actions->size(); ++action_idx) {
            if (actions->at(action_idx)->label == action_label) {
                return action_idx;
            }
        }
        return std::nullopt;
    }

    QValue MultiGather(std::vector<Successor*>& successors, std::vector<QValue*>& baseline, bool ignoreProbability = false);
    QValue MultiGather(const std::vector<QValue*>& worth, const std::vector<double> &probs, const std::vector<QValue*>& baseline, bool ignoreProbability=false);
    void AggregateWithCertainSuccessor(QValue& qval, Successor* scr);
    int CompareByTheories(QValue& qv1, QValue& qv2, bool useRanks=false);
    int CompareByConsiderations(QValue& qv1, QValue& qv2);
    int ParetoCompare(QValue& qv1, QValue& qv2);
    int ParetoCompare(QValue& qv1, QValue& qv2, std::vector<size_t>& consideration_indices);
    int compareExpectations(QValue& qv1, QValue& qv2, std::vector<int>& forwardTheories, std::vector<int>& reverseTheories);

    void blankQValue(QValue& qval);
    void getNoBaseLineQValue(State& state, int stateActionIndex, QValue& qval);
    void heuristicQValue(QValue& qval, State& state);
    bool isQValueInBudget(QValue& qval) const;

    bool checkPoliciesEqual(Policy& p1, Policy& p2);
    int checkPolicyInVector(Policy& pi, const std::vector<Policy*>& pols);
    std::vector<size_t> findPoliciesWithAction(std::vector<std::unique_ptr<Policy>>& pols, State& state, int actIdx);
};
