//
// Created by Simon Kolker on 30/08/2024.
//
#ifndef MEHR_H
#define MEHR_H
#include "Solution.hpp"
#include "Attack.hpp"
#include "iostream"
#include <unordered_set>

using namespace std;

class MEHR {
private:
    MDP& mdp;
    int mycount=0;
    void DFS_baseCase(QValue& qval, double long prob, unordered_map<QValue, double long, QValueHash, QValueEqual>& worthMap) {
        if (worthMap.find(qval) == worthMap.end())
            worthMap[qval] = prob;
        else
            worthMap[qval] += prob;
    }

    void DFS_Histories(State& state, int time, Solution& sol, QValue& qval, double long prob, unordered_map<QValue, double long, QValueHash, QValueEqual>& worthMap) {
        // Base Case. Stop when reaching the horizon.
        if (time >= mdp.horizon) {
            DFS_baseCase(qval, prob, worthMap);
            return;
        }
        int stateAction = sol.policy[time][state.id];
        if (stateAction == -1) {
            DFS_baseCase(qval, prob, worthMap);
            return;
        }
        auto successors = mdp.getActionSuccessors(state, stateAction);
        if (successors==nullptr) {
            DFS_baseCase(qval, prob, worthMap);
            return;
        }
        // Recursive Case.
        for (Successor* successor : *successors) {
            QValue qval_ = QValue(qval);
            mdp.addCertainSuccessorToQValue(qval_, successor);
            DFS_Histories(*mdp.states[successor->target], time+1, sol, qval_, prob*successor->probability, worthMap);
        }
    }


  public:
    MEHR(MDP& mdp) : mdp(mdp) { }
    ~MEHR() {

    }
    // Main algorithm
    vector<double long>* solve(MDP& mdp, vector<shared_ptr<Solution>>& solSet) {
        this->mdp = mdp;
        auto solExpectations = new vector<QValue>();
        auto outcomeWorths = new vector<shared_ptr<vector<QValue>>>;
        auto outcomeProbability = new vector<shared_ptr<vector<double long>>>;
        extractInfo(solSet, *solExpectations, *outcomeWorths, *outcomeProbability);

        // Get Non-Acceptability through Machine-Ethics-Hypothetical-Retrospection.
        auto na = findNonAccept(*solExpectations, *outcomeWorths, *outcomeProbability);
        delete solExpectations;
        delete outcomeWorths;
        delete outcomeProbability;

        return na;

    }
    // Load solution expectations, outcome expectations, and the outcome probabilities.
    void extractInfo(vector<shared_ptr<Solution>>& solSet, vector<QValue>& solExpectations, vector<shared_ptr<vector<QValue>>>& outcomeWorth, vector<shared_ptr<vector<double long>>>& outcomeProbs) {
        // Extract information from solution set.
        for (shared_ptr<Solution> sol : solSet) {
            // Add Policy expectation
            solExpectations.push_back(QValue());
            sol->loadQValue(*mdp.states[0], 0, solExpectations[solExpectations.size()-1]);
            // Add/Extract History outcomes
            shared_ptr<vector<QValue>> ows = make_shared<vector<QValue>>();
            shared_ptr<vector<double long>> ops = make_shared<vector<double long>>();

            extractHistoryWorths(sol, ows, ops);
            mycount++;
            outcomeWorth.push_back(ows);
            outcomeProbs.push_back(ops);
        }
    }
    void extractHistoryWorths(shared_ptr<Solution>& sol, shared_ptr<vector<QValue>> worths, shared_ptr<vector<double long>> probs) {
        QValue qval = QValue();
        mdp.blankQValue(qval);
        auto worthMap = unordered_map<QValue, double long, QValueHash, QValueEqual>();
        DFS_Histories(*mdp.states[0], 0, *sol, qval, 1, worthMap);
        for (auto& it: worthMap) {
            worths->push_back(it.first);
            probs->push_back(it.second);
        }

    }
    vector<double long>* findNonAccept(vector<QValue>& expectedWorths, vector<shared_ptr<vector<QValue>>>& outcomeWorths, vector<shared_ptr<vector<double long>>>& probability);
    Attack biRetrospect(QValue* outOne, QValue* expOne, double long probOne, QValue* outTwo, QValue* expTwo, double long probTwo);
    void verifyInput(vector<QValue*>& expOne, vector<vector<QValue*>>&& outsOne, vector<vector<double long>>&& probOne,
        vector<QValue*>& expTwo, vector<vector<QValue*>>& outsTwo, vector<vector<double long>>& probTwo) {
        if (expOne.size() != outsOne.size() || outsOne.size() != probOne.size())
            throw runtime_error("MEHR::verifyInput size mismatch between expectations and arguments/probabilities");
        if (expTwo.size() != outsTwo.size() || outsTwo.size() != probTwo.size())
            throw runtime_error("MEHR::verifyInput size mismatch between expectations and arguments/probabilities");

        for (int i = 0; i < outsOne.size(); i++) {
            if (outsOne[i].size() != probOne[i].size()) {
                throw runtime_error("MEHR::verifyInput size mismatch between arguments and probabilities");
            }
        }
        for (int i = 0; i < outsTwo.size(); i++) {
            if (outsOne[i].size() != probTwo[i].size()) {
                throw runtime_error("MEHR::verifyInput size mismatch between arguments and probabilities");
            }
        }
    }
    void checkForAttack(int sourceSol, int targetSol, vector<shared_ptr<vector<QValue>>>& outcomeWorths, vector<shared_ptr<vector<double long>>>& probability, vector<vector<int*>>& attackedOutcomes, vector<double long>* non_accept, vector<int>& theories);

};



#endif //MEHR_H
