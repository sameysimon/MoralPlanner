//
// Created by Simon Kolker on 22/10/2024.
//

#ifndef EXTRACTSOLUTIONS_HPP
#define EXTRACTSOLUTIONS_HPP

#include "Solver.hpp"
#include <memory>
#include <iostream>
#include <vector>

using namespace std;


class SolutionExtracter {

public:

    explicit SolutionExtracter(MDP& _mdp) : mdp(_mdp) {
        // Actions to search on each state.
        searchedOptions = new unordered_map<int, int>();
    }
    ~SolutionExtracter() {
        delete searchedOptions;
    }
    vector<Policy*>* extractPolicies(vector<vector<int>>& Pi, const vector<int>& Z) {
        // TODO Maybe don't keep whole policy table, delete far back, only keep time+1
        // Maps time-state to set of policies from that epoch.
        policyTable = new unordered_map<int, unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*>();

        // Stores of successor-to-sub policies.
        vector<unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*> tmpPolicies;
        auto tmpPolicyIndex = vector<vector<Policy*>*>();

        // Post-Order Depth-First-Search (bottom-up) building policies
        for (const int stateIdx : Z) {
            int time = mdp.states[stateIdx]->time;

            auto myPolicies = new unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>();
            (*policyTable)[stateIdx] = myPolicies;

            auto actionsDone = vector<int>();
            for (const auto stateAction : Pi[stateIdx]) {
                // Skip duplicate actions
                if (std::find(actionsDone.begin(), actionsDone.end(),stateAction)!=std::end(actionsDone)) { continue; }
                actionsDone.push_back(stateAction);

                //
                // Step 1. Get sub-policies of successors into tmpPolicies
                //
                // Get successors
                auto successors = mdp.getActionSuccessors(*mdp.states[stateIdx], stateAction);

                // If successor has no sub-policies, make them.
                if (time+1 >= mdp.horizon) {
                    // Next successor will not have any sub policies.
                    // Create policy without sub policies.
                    auto newPi = new Policy();
                    // Add successor's worth
                    for (const auto scr : *successors) {
                        QValue qval = QValue();
                        mdp.blankQValue(qval);
                        mdp.heuristicQValue(qval, *mdp.states[scr->target]);
                        newPi->setWorth(time+1, scr->target, qval);
                    }
                    // Add current state worth and aggregate.
                    newPi->setAction(time, stateIdx, stateAction);
                    QValue qval = gatherQValue(successors, *newPi, time);
                    newPi->setWorth(time, stateIdx, qval);
                    if (myPolicies->find(newPi)==myPolicies->end()) {
                        myPolicies->insert(newPi);
                        //std::cout << "   Adding policy: "<< newPi->toString() << endl;
                    }
                    continue;
                }
                // Fill tmpPolicies with sub-policies.
                getSubPolicies(time, tmpPolicies, successors);

                //
                // Step 2. Merge Policies.
                //
                // Create indexable vector of policies instead of unordered_set. Probs better methods...
                // tmpPolicyIndex[scrIdx] = [policies]
                tmpPolicyIndex.clear();
                for (int scrIdx=0; scrIdx< tmpPolicies.size(); scrIdx++) {
                    tmpPolicyIndex.push_back(new vector<Policy*>());
                    tmpPolicyIndex[scrIdx]->insert(tmpPolicyIndex[scrIdx]->end(), tmpPolicies[scrIdx]->begin(), tmpPolicies[scrIdx]->end());
                }

                // Get all combinations of successor's sub-policies.
                auto policyCombos = vector<vector<int>>();//Stores lists of successor's sub-policy' idxs.
                auto currentcombo = vector<int>(successors->size());// Stores a current combo from above (for algo purposes).
                generatePolicyCombos(policyCombos, tmpPolicyIndex, currentcombo, 0);

                for (auto& combo : policyCombos) {
                    auto newPi = new Policy();
                    // Copy values for sub-policies of each successor.
                    for (int scr_i = 0; scr_i < successors->size(); scr_i++) {
                        newPi->importPolicy(*tmpPolicyIndex[scr_i]->at(combo[scr_i]));// Copy in successor's policy to this new one.
                    }
                    // Aggregate combo's successor qValues with action.
                    newPi->setAction(time, stateIdx, stateAction);
                    QValue qval = gatherQValue(successors, *newPi, time);
                    newPi->setWorth(time, stateIdx, qval);

                    if (myPolicies->find(newPi)== myPolicies->end()) {
                        myPolicies->insert(newPi);
                    }
                }

                // Delete indexable version
                for (auto vec : tmpPolicyIndex ) { delete vec; }
                //cout << "   There are " << myPolicies->size() << " policies after action " << stateAction << endl;
                // Copy policy and set action/Value
            }
#ifdef DEBUG
            cout << "   Actions= " << actionsDone.size() << " unique actions" << endl;
            cout << "   Policies " << myPolicies->size() << endl;
#endif
        }
        //
        // Tidy up
        //
        // Save the polcies we need (which are the ones at state-time 0).
        unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>* solns = policyTable->at(0);

        // Tidy up memorized policies.
        for (auto elem : *policyTable ) {
            if (elem.first==0) { continue; }
            for (auto pi : *elem.second) {
                delete pi;
            }
        }
        delete policyTable;

        // Convert set to vector (that is in budget)
        auto result = new vector<Policy*>();
        bool existsPolicyInBudget = std::any_of(solns->begin(), solns->end(), [this](Policy* pi) {
            return this->mdp.isQValueInBudget(pi->worth[0]);
        });

        if (existsPolicyInBudget) {
            // Only copy those in budget, if any are in budget.
            std::copy_if(solns->begin(), solns->end(), std::back_inserter(*result), [this](Policy* pi) {
                return this->mdp.isQValueInBudget(pi->worth[0]);
            });
        } else {
            // If none are in budget, copy all solutions.
            result->insert(result->end(), solns->begin(), solns->end());
        }
        return result;
    }

    string policiesToString(vector<Policy*>& policies) {
        stringstream ss;
        int myCounter=0;
        for (auto pi : policies) {
            ss << myCounter << ". " << pi->toString(mdp) << endl;
            myCounter++;
        }
        return ss.str();
    }

    void getSubPolicies(int time, vector<unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*>& tmpPolicies, vector<Successor*>* successors) {
        tmpPolicies.clear();
        // Successor Idx to unique sub policies for this action.
        for (auto* scr : *successors) {
            // Get successor's unique policies
            unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>* subPolicies = (*policyTable)[scr->target];
            // If no policies, make new empty-valued for action.
            if (subPolicies->empty()) {
                auto newPi = new Policy();
                // Set to blank value at time+1, scr->target
                QValue qval = QValue();
                mdp.blankQValue(qval);
                mdp.heuristicQValue(qval, *mdp.states[scr->target]);

                newPi->setWorth(time+1, scr->target, qval);
                subPolicies->insert(newPi);
                std::cout << "   No existing sub policies, for successor " << scr->target << " so made one" << endl;
            }
            tmpPolicies.push_back(subPolicies);
        }
    }

private:
    MDP& mdp;
    unordered_map<int, int>* searchedOptions;
    unordered_map<int, unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*>* policyTable;

    QValue gatherQValue(vector<Successor*>* successors, Policy& pi, int currentTime) {
        QValue new_qv = QValue();
        mdp.blankQValue(new_qv);
        std::vector<WorthBase*> baselines = std::vector<WorthBase*>(successors->size());

        for (int theoryIdx = 0; theoryIdx < mdp.theories.size(); ++theoryIdx) {
            for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
                baselines[scrIdx] = pi.getWorthAtTheory(currentTime+1, (*successors)[scrIdx]->target, theoryIdx);
            }
            new_qv.expectations[theoryIdx] = mdp.theories[theoryIdx]->gather(*successors,baselines);
        }
        return new_qv;
    }
    void generatePolicyCombos(vector<vector<int>>& allCombos, vector<vector<Policy*>*>& successorPis, vector<int>& curr, int idx) {
        if (idx==successorPis.size()) {
            allCombos.push_back(curr);
            return;
        }
        for (int scr_i = 0; scr_i < successorPis[idx]->size(); scr_i++) {
            curr[idx] = scr_i;
            generatePolicyCombos(allCombos, successorPis, curr, idx+1);
        }

    }
};

#endif
