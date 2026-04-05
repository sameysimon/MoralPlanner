//
// Created by Simon Kolker on 22/10/2024.
//

#pragma once

#include "Solver.hpp"
#include "History.hpp"
#include "../Logger.hpp"
#include <iostream>
#include <vector>

using namespace std;


class SolutionExtracter {
    MDP& mdp;
    bool make_history_paths = false;
    unordered_map<size_t, vector<size_t>> forced_actions;
public:
    explicit SolutionExtracter(MDP& _mdp, bool make_history_paths_=false) : mdp(_mdp), make_history_paths(make_history_paths_) { }

    void ForceStateAction(size_t state_idx, size_t action_idx) {
        forced_actions.insert_or_assign(state_idx, vector<size_t>());
        forced_actions[state_idx].push_back(action_idx);
    }

    typedef unordered_set<unique_ptr<Policy>, PolicyPtrHash, PolicyPtrEqual> policy_set;

    void Extract(vector<unique_ptr<Policy>>& result, vector<vector<unique_ptr<History>>> &histories, vector<vector<int>>& Pi) {
        auto piTable = array<vector<policy_set>, 2>();
        auto piStateTable = array<vector<size_t>, 2>(); // piTable[0 or 1][i] = set of policies for state piStateTable[i].
        int currTime = 0;
        int prevPolicies = 0; // index of previous policies in piTable.
        int currStateSetIdx = 0;
        vector<size_t> all_states(mdp.states.size());
        std::iota(all_states.begin(), all_states.end(), 0);
        std::sort(all_states.begin(), all_states.end(), [this](size_t a, size_t b) {
            return mdp.states[a]->time > mdp.states[b]->time;
        });
        for (const size_t stateIdx : all_states) {
            if (mdp.states[stateIdx]->time < currTime) {
                piTable[prevPolicies].clear();
                piStateTable[prevPolicies].clear();
                prevPolicies = 1 - prevPolicies;
                currStateSetIdx = 0;
            }
            currTime = mdp.states[stateIdx]->time;
            piStateTable[1-prevPolicies].push_back(stateIdx);
            if (((int)piTable[1-prevPolicies].size()) - 1 < currStateSetIdx) {
                piTable[1-prevPolicies].emplace_back(); // instantiates unordered_set for curr_state

            }
            piTable[1-prevPolicies][currStateSetIdx].clear();

            auto stateActions = vector<int>();
            for (const auto a : Pi[stateIdx]) {
                if (find(stateActions.begin(), stateActions.end(), a) != end(stateActions)) {
                    continue;
                }
                stateActions.push_back(a);

                auto successors = MDP::getActionSuccessors(*mdp.states[stateIdx], a);
                auto scrPolicyCombos = GetSuccessorPolicyCombos(successors, piTable[prevPolicies], piStateTable[prevPolicies]);

                for (auto &combo : scrPolicyCombos) {
                    auto newPi = make_unique<Policy>(mdp, stateIdx);
                    // Copy values for sub-policies of each successor.
                    newPi->MergePolicies(combo, *successors, mdp);
                    // Aggregate combo's successor qValues with action.
                    newPi->addAction((int)stateIdx, a);
                    gatherQValue(newPi->AddWorth(mdp, stateIdx), successors, *newPi, currTime);

                    auto &tab = piTable[1-prevPolicies][currStateSetIdx];
                    auto [s, inserted] = tab.insert(std::move(newPi));
                    if (!inserted) {
                        s->get()->included_state_actions.emplace_back(stateIdx, a);
                    }
                }
            }
            currStateSetIdx++;
        }
        auto &solns = piTable[1 - prevPolicies][0];
        result.reserve(solns.size());
        for (auto it = solns.begin(); it != solns.end(); ) {
            auto policynh = solns.extract(it++);
            result.push_back(std::move(policynh.value()));
            histories.emplace_back();
            histories.back().reserve(result.back()->history_set.size());
            auto &hs = result.back()->history_set;
            for (auto it = hs.begin(); it != hs.end(); ) {
                auto histnh = hs.extract(it++);
                histories.back().emplace_back(std::move(histnh.value()));
            }
            result.back()->history_set.clear();
        }
    }

    vector<vector<Policy*>>
    GetSuccessorPolicyCombos(vector<Successor*>* successors, vector<policy_set>& piTable, vector<size_t>& piStateLookup) {
        // Worth vector for each combination of policies useful to successors.
        vector<vector<Policy*>> combs(1);
        vector<size_t> scr_states;
        for (auto& scr : *successors) {
            size_t target = scr->target;
            if (find(scr_states.begin(), scr_states.end(), target) != scr_states.end()) {
                continue;
            }
            scr_states.push_back(target);
            // Are there policies for this successor?
            auto scrPolicyVecIt = std::find_if(piStateLookup.begin(), piStateLookup.end(), [target](int n) {
                return target==n;
            });
            size_t scrPolicyVecIdx = std::distance(std::begin(piStateLookup), scrPolicyVecIt);
            // If no policies exist for successor, generate one.
            if (scrPolicyVecIt == piStateLookup.end() || piTable[scrPolicyVecIdx].empty()) {
                auto newPi = make_unique<Policy>(mdp, target);
                auto& qval = newPi->AddWorth(mdp, static_cast<int>(target));
                mdp.blankQValue(qval);
                mdp.heuristicQValue(qval, *mdp.states[target]);
                newPi->history_set.insert(make_unique<History>(mdp, 1, make_history_paths));
                if (scrPolicyVecIt == piStateLookup.end()) {
                    scrPolicyVecIdx = piStateLookup.size();
                    piTable.emplace_back();
                    piStateLookup.emplace_back(target);
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

    string stringify(vector<unique_ptr<Policy>>& pols, MDP &mdp) {
        string s;
        for (size_t i=0; i < pols.size(); ++i) {
            auto &pi = *pols[i];
            s += format("Policy {}\n", i);
            s += pi.toString();
        }
        return s;
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
            //new_qv.expectations[theoryIdx] = mdp.considerations[theoryIdx]->gather(*successors,baselines, false);
        }


        std::vector<QValue*> baselines_ = std::vector<QValue*>(successors->size());
        for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
            baselines_[scrIdx] = &pi.worth[(*successors)[scrIdx]->target];
        }
        new_qv = mdp.MultiGather(*successors, baselines_);
}
};

