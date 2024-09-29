//
// Created by Simon Kolker on 30/08/2024.
//
#ifndef MEHR_H
#define MEHR_H
#include "Solution.hpp"
#include "Attack.hpp"
#include <algorithm>
#include <unordered_set>

using namespace std;

class MEHR {
private:
    MDP& mdp;
    void DFS_baseCase(QValue& qval, double prob, unordered_map<QValue, double, QValueHash, QValueEqual>& worthMap) {
        if (worthMap.find(qval) == worthMap.end())
            worthMap[qval] = prob;
        else
            worthMap[qval] += prob;
    }
    void DFS_Histories(State& state, int time, Solution& sol, QValue& qval, double prob, unordered_map<QValue, double, QValueHash, QValueEqual>& worthMap) {
        // Base Case. Stop when reaching the horizon.
        if (time >= mdp.horizon) {
            DFS_baseCase(qval, prob, worthMap);
            return;
        }
        auto successors = mdp.getActionSuccessors(state, sol.policy[time][state.id]);
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
    // Main algorithm
    vector<double>* solve(MDP& mdp, vector<shared_ptr<Solution>>& solSet) {
        this->mdp = mdp;
        auto solExpectations = vector<QValue>();
        auto outcomeWorths = vector<vector<QValue>*>();
        auto outcomeProbability = vector<vector<double>*>();
        extractInfo(solSet, solExpectations, outcomeWorths, outcomeProbability);

        // Get Non-Acceptability through Machine-Ethics-Hypothetical-Retrospection.
        return findNonAccept(solExpectations, outcomeWorths, outcomeProbability);

    }
    // Load solution expectations, outcome expectations, and the outcome probabilities.
    void extractInfo(vector<shared_ptr<Solution>>& solSet, vector<QValue>& solExpectations, vector<vector<QValue>*>& outcomeWorth, vector<vector<double>*>& outcomeProbs) {
        // Extract information from solution set.
        for (shared_ptr<Solution> sol : solSet) {
            // Add Policy expectation
            solExpectations.push_back(QValue());// TODO Initialise QValue
            sol->loadQValue(*mdp.states[0], 0, solExpectations[solExpectations.size()-1]);
            // Add/Extract History outcomes
            vector<QValue>* ows = new vector<QValue>();
            vector<double>* ops = new vector<double>();
            extractHistoryWorths(sol, ows, ops);
            outcomeWorth.push_back(ows);
            outcomeProbs.push_back(ops);
        }
    }
    void extractHistoryWorths(shared_ptr<Solution>& sol, vector<QValue>* worths, vector<double>* probs) {
        QValue qval = QValue();
        mdp.blankQValue(qval);
        auto worthMap = unordered_map<QValue, double, QValueHash, QValueEqual>();
        DFS_Histories(*mdp.states[0], 0, *sol, qval, 1, worthMap);
        for (auto& it: worthMap) {
            worths->push_back(it.first);
            probs->push_back(it.second);
        }

    }

    vector<double>* findNonAccept(vector<QValue>& expectedWorths, vector<vector<QValue>*>& outcomeWorths, vector<vector<double>*>& probability);
    Attack biRetrospect(QValue* outOne, QValue* expOne, double probOne, QValue* outTwo, QValue* expTwo, double probTwo);
    void verifyInput(vector<QValue*>& expOne, vector<vector<QValue*>>&& outsOne, vector<vector<double>>&& probOne,
        vector<QValue*>& expTwo, vector<vector<QValue*>>& outsTwo, vector<vector<double>>& probTwo) {
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




};



#endif //MEHR_H
