//
//  MDP.hpp
//  MPlan
//
//  Created by e56834sk on 10/07/2024.
//

#ifndef MDP_hpp
#define MDP_hpp

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
#include <set>
#include "QValue.hpp"

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
    size_t AddMEHRTheory(MEHRTheory *mehrTheory, int rank, std::set<int> &unique_ordered_ranks);
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

    static std::vector<Successor*>* getActionSuccessors(const State &state, const int stateActionIndex) {
        if (state.actionSuccessors.size()==0) {
            throw std::runtime_error(std::format("getActionSuccessors called with state {} and state-action Index {} has no successors.", state.id, stateActionIndex));
        }
         return state.actionSuccessors[stateActionIndex];
    }
    std::vector<Successor*>* getActionSuccessors(const State& state, const Action& action) {
        // Finds state-action index of 'action' then returns its successors.
        int i = 0;
        for (auto a : *getActions(state)) {
            if (a->label == action.label) { //EW TODO Make whole thing better!!!!
                return getActionSuccessors(state, i);
            }
            i++;
        }
        throw std::runtime_error("No suitable action found.");
    }
    std::vector<std::shared_ptr<Action>>* getActions(const State& state) {
        return &(stateActions[state.id]);
    }
    void addCertainSuccessorToQValue(QValue& qval, Successor* scr);
    int CompareByTheories(QValue& qv1, QValue& qv2, bool useRanks=false);
    int CompareByConsiderations(QValue& qv1, QValue& qv2);
    int compareExpectations(QValue& qv1, QValue& qv2, std::vector<int>& forwardTheories, std::vector<int>& reverseTheories);

    void blankQValue(QValue& qval);
    void getNoBaseLineQValue(State& state, int stateActionIndex, QValue& qval);
    void heuristicQValue(QValue& qval, State& state);
    bool isQValueInBudget(QValue& qval) const;

    bool checkPoliciesEqual(Policy& p1, Policy& p2);
    int checkPolicyInVector(const Policy& pi, const std::vector<Policy*>& pols);
    std::vector<size_t> findPoliciesWithAction(std::vector<Policy*>& pols, State& state, int actIdx);

};


#endif /* MDP_hpp */
