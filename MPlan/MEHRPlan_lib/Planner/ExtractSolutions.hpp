//
// Created by Simon Kolker on 22/10/2024.
//

#pragma once

#include "Solver.hpp"
#include "../Logger.hpp"
#include <iostream>
#include <vector>

using namespace std;


class SolutionExtracter {
    MDP& mdp;
    unordered_map<int, int>* searchedOptions;
    unordered_map<int, unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*>* policyTable;
public:
    explicit SolutionExtracter(MDP& _mdp) : mdp(_mdp) {
        // Actions to search on each state.
        searchedOptions = new unordered_map<int, int>();
    }
    ~SolutionExtracter() {
        delete searchedOptions;
    }


    void Extract(vector<unique_ptr<Policy>>& result, vector<vector<int>>& Pi, vector<int>& Z) {
        auto piTable = array<vector<unordered_set<unique_ptr<Policy>, PolicyPtrHash, PolicyPtrEqual>>, 2>();
        auto piStateTable = array<vector<size_t>, 2>(); // piTable[0 or 1][i] = set of policies for state piStateTable[i].
        int currTime = 0;
        int prevPolicies = 0; // index of previous policies in piTable.
        int currStatePiTable = 0;
        std::sort(Z.begin(), Z.end(), [this](size_t a, size_t b) {
            return mdp.states[a]->time > mdp.states[b]->time;
        });
        for (const int stateIdx : Z) {
            if (mdp.states[stateIdx]->time < currTime) {
                piTable[prevPolicies].clear();
                piStateTable[prevPolicies].clear();
                prevPolicies = 1 - prevPolicies;
                currStatePiTable = 0;
            }
            currTime = mdp.states[stateIdx]->time;
            piStateTable[1-prevPolicies].push_back(stateIdx);
            if (((int)piTable[1-prevPolicies].size()) - 1 < currStatePiTable) {
                piTable[1-prevPolicies].emplace_back(); // instantiates unordered_set for curr_state

            }
            piTable[1-prevPolicies][currStatePiTable].clear();

            auto stateActions = vector<int>();
            for (const auto a : Pi[stateIdx]) {
                auto findIter = std::find(stateActions.begin(), stateActions.end(), a);
                if (findIter != std::end(stateActions)) {
                    continue;
                }
                stateActions.push_back(a);
                auto successors = MDP::getActionSuccessors(*mdp.states[stateIdx], a);
                auto scrPolicyCombos = GetSuccessorPolicyCombos(successors, piTable[prevPolicies], piStateTable[prevPolicies]);

                for (auto &combo : scrPolicyCombos) {
                    // TODO Instantiating so many new policies is not great; would be better to modify existing. But set for uniqueness makes checking difficult.
                    //  Could have a marker on each policy saying this time step or the previous. It counts to the hash. Uniqueness only checked against current/matching mark.
                    //  Use hash-indexed set to check marked policy uniqueness and store old Policies to be updated.
                    auto newPi = make_unique<Policy>(mdp);
                    // Copy values for sub-policies of each successor.
                    for (int scr_i = 0; scr_i < successors->size(); scr_i++) {
                        newPi->importPolicy(*combo[scr_i]);// Copy in successor's policy to this new one.
                    }
                    // Aggregate combo's successor qValues with action.
                    newPi->setAction(stateIdx, a);
                    gatherQValue(newPi->AddWorth(mdp, stateIdx), successors, *newPi, currTime);

                    auto &tab = piTable[1-prevPolicies][currStatePiTable];
                    if (std::find(tab.begin(), tab.end(), newPi) == tab.end()) {
                        tab.insert(std::move(newPi));
                    }
                }
            }
            currStatePiTable++;
        }
        auto &solns = piTable[1 - prevPolicies][0];
        result.reserve(solns.size());
        for (auto it = solns.begin(); it != solns.end(); ) {
            auto nh = solns.extract(it++);
            result.push_back(std::move(nh.value()));
        }
    }

    vector<vector<Policy*>> GetSuccessorPolicyCombos(vector<Successor*>* successors, vector<unordered_set<unique_ptr<Policy>, PolicyPtrHash, PolicyPtrEqual>>& piTable, vector<size_t>& piStateLookup) {
        // Will vector for each combination of policies useful to successors.
        vector<vector<Policy*>> combs(1);
        // Iterate through the successors
        for (auto& scr : *successors) {
            // Are there policies for this successor?
            auto scrPolicyVecIt = std::find_if(piStateLookup.begin(), piStateLookup.end(), [scr](int n) {
                return scr->target==n;
            });
            auto scrPolicyVecIdx = std::distance(std::begin(piStateLookup), scrPolicyVecIt);
            // If no policies exist for successor, generate one.
            if (scrPolicyVecIt == piStateLookup.end() || piTable[scrPolicyVecIdx].empty()) {
                auto newPi = make_unique<Policy>(mdp);
                auto& qval = newPi->AddWorth(mdp, scr->target);
                mdp.blankQValue(qval);
                mdp.heuristicQValue(qval, *mdp.states[scr->target]);
                if (scrPolicyVecIt == piStateLookup.end()) {
                    scrPolicyVecIdx = piStateLookup.size();
                    piTable.emplace_back();
                    piStateLookup.emplace_back(scr->target);
                }
                piTable[scrPolicyVecIdx].insert(std::move(newPi));
            }

            size_t init_combo_size = combs.size();
            // Update existing combinations for this successor's first policy.
            // For every policy this successor uses past the first, copy previous combinations and add policy.
            auto scrPolicyIt = piTable[scrPolicyVecIdx].begin();
            ++scrPolicyIt;
            while (scrPolicyIt != piTable[scrPolicyVecIdx].end()) {
                for (size_t combo_idx = 0; combo_idx < init_combo_size; ++combo_idx) {
                    combs.push_back(combs[combo_idx]);
                    combs.back().push_back(scrPolicyIt->get());
                }
                ++scrPolicyIt;
            }
            // Add first policy this successor uses to the first combination.
            scrPolicyIt = piTable[scrPolicyVecIdx].begin();
            for (size_t comb_idx = 0; comb_idx < init_combo_size; ++comb_idx) {
                combs[comb_idx].push_back(scrPolicyIt->get());
            }
        }
        return combs;
    }


    void extractPolicies(vector<Policy*>& result, vector<vector<int>>& Pi, const vector<int>& Z) {/*
        // TODO Do not need the whole policy table; can delete far back, only keep time+1
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

                    Policy* pi = createSubPolicyAtRoot(stateIdx, stateAction, successors);
                    if (myPolicies->find(pi)==myPolicies->end()) {
                        myPolicies->insert(pi);
                    }
                    continue;
                    throw runtime_error("SolutionExtracter::MDP::getActionSuccessors() over horizon");
                }

                // Get sub-policies of successors.
                tmpPolicies.clear();
                getSubPolicies(tmpPolicies, successors);

                // Merge Policies.
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
                    auto newPi = new Policy(mdp);
                    // Copy values for sub-policies of each successor.
                    for (int scr_i = 0; scr_i < successors->size(); scr_i++) {
                        newPi->importPolicy(*tmpPolicyIndex[scr_i]->at(combo[scr_i]));// Copy in successor's policy to this new one.
                    }
                    // Aggregate combo's successor qValues with action.
                    newPi->setAction(stateIdx, stateAction);
                    gatherQValue(newPi->AddWorth(mdp, stateIdx), successors, *newPi, time);

                    if (myPolicies->find(newPi) == myPolicies->end()) {
                        myPolicies->insert(newPi);
                    } else {
                        delete newPi;
                    }
                }
                // prune policies here
                std::vector<int> indicesOfUndominated;

                for (auto vec : tmpPolicyIndex ) { delete vec; }
            }
#ifdef DEBUG
            Log::writeFormatLog(Trace, "{} unique actions.", actionsDone.size());
            Log::writeFormatLog(Trace, "{} policies.", myPolicies->size());
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
    */
    }

    Policy* createSubPolicyAtRoot(int stateIdx, int stateAction, vector<Successor*>* successors) {
        // Create policy without sub policies.
        auto newPi = new Policy(mdp);
        // Add heuristic worth of successors at successor values
        for (const auto scr : *successors) {
            QValue qval = newPi->AddWorth(mdp, scr->target);
            mdp.blankQValue(qval);
            mdp.heuristicQValue(qval, *mdp.states[scr->target]);
        }
        // Set action/worth of current state+time
        newPi->setAction(stateIdx, stateAction);
        gatherQValue(newPi->AddWorth(mdp, stateIdx), successors, *newPi, mdp.states[stateIdx]->time);

        return newPi;

    }

    // Get sets of sub policies rooted at successors, for each successor.
    // scrPolicies[scrIdx] = set of policies rooted at scrIdx.
    // If no sub-policies rooted at successor, create new blank policy with heuristic QValue at scrIdx.
    void getSubPolicies(vector<unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>*>& scrPolicies, vector<Successor*>* successors) {
        /*
        // Successor Idx to unique sub policies for this action.
        for (auto* scr : *successors) {
            // Get successor's unique policies
            unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>* subPolicies = (*policyTable)[scr->target];
            // If no policies, make new empty-valued for action.
            if (subPolicies->empty()) {
                auto newPi = new Policy(mdp);
                // Set to blank value at time+1, scr->target
                auto& qval = newPi->AddWorth(mdp, scr->target);
                mdp.blankQValue(qval);
                mdp.heuristicQValue(qval, *mdp.states[scr->target]);
                subPolicies->insert(newPi);
                Log::writeLog(std::format("No existing sub-policies for successor {} so made one.", scr->target), Debug);
            }
            scrPolicies.push_back(subPolicies);
        }*/
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

    bool isDominated(int stateIdx, QValue& qval, unordered_set<Policy*, PolicyPtrHash, PolicyPtrEqual>& policies) {
        for (auto pi : policies) {
            if (mdp.ParetoCompare(qval, pi->worth[stateIdx])==-1) {
                return true;
            }
        }
        return false;
    }

    void gatherQValue(QValue& new_qv, vector<Successor*>* successors, Policy& pi, int currentTime) {
        std::vector<WorthBase*> baselines = std::vector<WorthBase*>(successors->size());
        for (int theoryIdx = 0; theoryIdx < mdp.considerations.size(); ++theoryIdx) {
            for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
                baselines[scrIdx] = pi.getWorthAtTheory(currentTime+1, (*successors)[scrIdx]->target, theoryIdx);
            }
            new_qv.expectations[theoryIdx] = mdp.considerations[theoryIdx]->gather(*successors,baselines, false);
        }
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

