//
// Created by Simon Kolker on 22/10/2024.
//

#ifndef EXTRACTSOLUTIONS_HPP
#define EXTRACTSOLUTIONS_HPP

#include "Solver.hpp"
#include "../Logger.hpp"
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
    vector<Policy*>* extractPolicies(vector<Policy*>& result, vector<vector<int>>& Pi, const vector<int>& Z) {
        // TODO Maybe don't keep whole policy table, delete far back, only keep time+1
        // Maps state to set of policies from that epoch.
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
                if (std::find(actionsDone.begin(), actionsDone.end(), stateAction)!=std::end(actionsDone)) {
                    continue;
                }
                actionsDone.push_back(stateAction);

                auto successors = MDP::getActionSuccessors(*mdp.states[stateIdx], stateAction);

                if (time+1 >= mdp.horizon) {
                    // This state takes no actions. It will have no meaningful sub policies.
                    // Make a dummy one though
                    /*
                    Policy* pi = createSubPolicyAtRoot(stateIdx, stateAction, successors);
                    if (myPolicies->find(pi)==myPolicies->end()) {
                        myPolicies->insert(pi);
                    }
                    continue;*/
                    throw runtime_error("SolutionExtracter::MDP::getActionSuccessors() over horizon");
                }


                // Get sub-policies of successors.
                tmpPolicies.clear();
                getSubPolicies(tmpPolicies, successors);

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

                //generatePolicyCombos(policyCombos, tmpPolicyIndex);

                for (auto& combo : policyCombos) {
                    auto newPi = new Policy();
                    // Copy values for sub-policies of each successor.
                    for (int scr_i = 0; scr_i < successors->size(); scr_i++) {
                        newPi->importPolicy(*tmpPolicyIndex[scr_i]->at(combo[scr_i]));// Copy in successor's policy to this new one.
                    }
                    // Aggregate combo's successor qValues with action.
                    newPi->setAction(stateIdx, stateAction);
                    QValue qval = gatherQValue(successors, *newPi, time);
                    newPi->setWorth(stateIdx, qval);

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
        bool existsPolicyInBudget = std::any_of(solns->begin(), solns->end(), [this](Policy* pi) {
            return this->mdp.isQValueInBudget(pi->worth[0]);
        });

        if (existsPolicyInBudget) {
            // Only copy those in budget, if any are in budget.
            std::copy_if(solns->begin(), solns->end(), std::back_inserter(result), [this](Policy* pi) {
                return this->mdp.isQValueInBudget(pi->worth[0]);
            });
        } else {
            // If none are in budget, copy all solutions.
            result.insert(result.end(), solns->begin(), solns->end());
        }
    }

    Policy* createSubPolicyAtRoot(int stateIdx, int stateAction, vector<Successor*>* successors) {
        // Create policy without sub policies.
        auto newPi = new Policy();
        // Add heuristic worth of successors at successor values
        for (const auto scr : *successors) {
            QValue qval = QValue();
            mdp.blankQValue(qval);
            mdp.heuristicQValue(qval, *mdp.states[scr->target]);
            newPi->setWorth(scr->target, qval);
        }
        // Set action/worth of current state+time
        newPi->setAction(stateIdx, stateAction);
        QValue qval = gatherQValue(successors, *newPi, mdp.states[stateIdx]->time);
        newPi->setWorth(stateIdx, qval);
        return newPi;

    }


    // Get sets of sub policies rooted at successors, for each successor.
    // scrPolicies[scrIdx] = set of policies rooted at scrIdx.
    // If no sub-policies rooted at successor, create new blank policy with heuristic QValue at scrIdx.
    void getSubPolicies(vector<unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*>& scrPolicies, vector<Successor*>* successors) {
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

                newPi->setWorth(scr->target, qval);
                subPolicies->insert(newPi);
                Log::writeLog(std::format("No existing sub-policies for successor {} so made one.", scr->target));
            }
            scrPolicies.push_back(subPolicies);
        }
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



private:
    MDP& mdp;
    unordered_map<int, int>* searchedOptions;
    unordered_map<int, unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*>* policyTable;

    QValue gatherQValue(vector<Successor*>* successors, Policy& pi, int currentTime) {
        QValue new_qv = QValue();
        mdp.blankQValue(new_qv);
        std::vector<WorthBase*> baselines = std::vector<WorthBase*>(successors->size());

        for (int theoryIdx = 0; theoryIdx < mdp.considerations.size(); ++theoryIdx) {
            for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
                baselines[scrIdx] = pi.getWorthAtTheory(currentTime+1, (*successors)[scrIdx]->target, theoryIdx);
            }
            new_qv.expectations[theoryIdx] = mdp.considerations[theoryIdx]->gather(*successors,baselines, false);
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

    // Generate combinations of successor sub-policies.
    // If subPolicies[scr_1] = [pi_1, pi_2] and subPolicies[scr_2] = [pi_3, pi_4]
    // Then allCombos is [[pi_1, pi_3], [pi_1, pi_4], [pi_2, pi_3], [pi_2, pi_4]].
    static void generatePolicyCombos(vector<vector<int>>& allCombos, vector<vector<Policy*>*>& subPolicies) {
        int total_Successors = (int)subPolicies.size();
        if (total_Successors == 0) return;

        // Initialize current combination with all zeros.
        vector<int> curr(total_Successors, 0);

        while (true) {
            // Start from the last index and find one to increment.
            int idx = total_Successors - 1;
            while (curr[idx] == subPolicies[idx]->size() - 1) {
                idx--; // Move left if this index has reached its max value
                if (idx < 0) { break; } // All positions have reached their maximum values
            }

            // Increment the found index.
            curr[idx]++;
            // Reset all indices to the right to zero.
            for (int j = idx + 1; j < total_Successors; ++j) {
                curr[j] = 0;
            }

            allCombos.push_back(curr);
        }
    }
};

#endif
