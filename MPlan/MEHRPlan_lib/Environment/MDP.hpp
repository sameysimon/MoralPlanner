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
#include "QValue.hpp"

class QValue;

class MDP {
    int test;
    std::unordered_map<std::string, Action*> actionMap;
    std::vector<std::vector<Action*>> stateActions;
    // Groups of moral theory indices, first holds lowest ranked theories.
    std::vector<std::vector<int>> groupedTheoryIndices;
    void buildFromJSON(nlohmann::json& data);
    int compareQValueByRank(QValue& qv1, QValue& qv2, int rank);
public:
    std::vector<Action*> actions;
    std::vector<State*> states;
    std::vector<MoralTheory*> theories;// Should not change size after initial assignment.
    int total_states=0;
    int horizon=3;
    float budget = -1;// initial budget is infinite
    int non_moralTheoryIdx=-1;

    explicit MDP(const std::string& fileName);
    explicit MDP(nlohmann::json& data);
    ~MDP();

    std::vector<Successor*>* getActionSuccessors(const State& state, const int stateActionIndex) {
        if (state.actionSuccessors.size()==0) {
            return nullptr;
        }
         return state.actionSuccessors[stateActionIndex];
    }
    std::vector<Successor*>* getActionSuccessors(const State& state, const Action& action) {
        // Finds state-action index of 'action' then returns its successors.
        int i = 0;
        for (auto a : *getActions(state, 0)) {
            if (a->label == action.label) { //EW TODO Make whole thing better!!!!
                return getActionSuccessors(state, i);
            }
            i++;
        }
        throw std::runtime_error("No suitable action found.");
    }
    std::vector<Action*>* getActions(const State& state, int timeStamp) {
        if (timeStamp > horizon) {
            throw std::runtime_error("Requested actions for timestamp after end of horizon.");
        }
        return &(stateActions[state.id]);
    }
    void addCertainSuccessorToQValue(QValue& qval, Successor* scr);
    int compareQValues(QValue& qv1, QValue& qv2, bool useRanks=false);
    int countAttacks(QValue& qv1, QValue& qv2);
    void blankQValue(QValue& qval);
    bool checkInBudget(QValue& qval);
    int compareExpectations(QValue& qv1, QValue& qv2, std::vector<int>& forwardTheories, std::vector<int>& reverseTheories);




};


#endif /* MDP_hpp */
