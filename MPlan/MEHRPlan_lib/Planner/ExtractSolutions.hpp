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

typedef unordered_set<unique_ptr<Policy>, PolicyPtrHash, PolicyPtrEqual> policy_set;

class SolutionExtracter {
    MDP& mdp;
    bool make_history_paths = false;
    unordered_map<size_t, vector<size_t>> forced_actions;
public:
    bool prune_dominated = true;
    explicit SolutionExtracter(MDP& _mdp, bool make_history_paths_=false) : mdp(_mdp), make_history_paths(make_history_paths_) { }

    void ForceStateAction(size_t state_idx, size_t action_idx) {
        forced_actions.insert_or_assign(state_idx, vector<size_t>());
        forced_actions[state_idx].push_back(action_idx);
    }

    void Extract(vector<unique_ptr<Policy>>& result, vector<vector<unique_ptr<History>>> &histories, vector<vector<int>>& Pi) {
        // For some time t and t+1, stores set of policies for each state.
        auto piTable = array<vector<policy_set>, 2>();
        // For same time t and t+1, stores state index for sets of policies in piTable
        auto piStateTable = array<vector<size_t>, 2>(); // piTable[0 or 1][i] = set of policies for state piStateTable[i].

        int currTime = 0;
        int prevPolicies = 0; // index of previous policies in piTable.
        int currStateSetIdx = 0;

        // Order states by time:
        vector<size_t> state_order(mdp.states.size());
        std::iota(state_order.begin(), state_order.end(), 0);
        std::sort(state_order.begin(), state_order.end(), [this](size_t a, size_t b) {
            return mdp.states[a]->time > mdp.states[b]->time;
        });

        for (const size_t stateIdx : state_order) {
            // Check for new time step; rotate piTable
            if (mdp.states[stateIdx]->time < currTime) {
                piTable[prevPolicies].clear();
                piStateTable[prevPolicies].clear();
                prevPolicies = 1 - prevPolicies;
                currStateSetIdx = 0;
            }
            // Update time and policy create set for current state.
            currTime = mdp.states[stateIdx]->time;
            piStateTable[1-prevPolicies].push_back(stateIdx);
            if (((int)piTable[1-prevPolicies].size()) - 1 < currStateSetIdx) {
                piTable[1-prevPolicies].emplace_back(); // instantiates unordered_set for curr_state
            }
            piTable[1-prevPolicies][currStateSetIdx].clear();

            // Pareto filter Policy candidates
            auto stateActions = vector<int>();
            vector<QValue> curr_QValues;
            vector<int> action_index;
            struct Candidate {
                int action_idx;
                vector<Policy*> children;
                QValue root;
                vector<Successor*>* successors;
            };
            list<Candidate> pcsCandidates;
            bool any_in_budget = false;
            if (mdp.non_moralTheoryIdx==-1) {
                any_in_budget = true;
            }
            // Iterate through PF actions at current state
            for (const auto a : Pi[stateIdx]) {
                // Skip copied actions
                if (find(stateActions.begin(), stateActions.end(), a) != end(stateActions)) { continue; }
                stateActions.push_back(a);

                auto successors = MDP::getActionSuccessors(*mdp.states[stateIdx], a);
                auto scrPolicyCombos = GetSuccessorPolicyCombos(successors, piTable[prevPolicies], piStateTable[prevPolicies]);
                for (auto &combo : scrPolicyCombos) {
                    Candidate c = {a,combo, QValue(mdp), successors};
                    gatherQValue(c.root, successors, combo, currTime);
                    if (!prune_dominated) {
                        pcsCandidates.push_back(c);
                        continue;
                    }
                    Solver::ParetoFilter(mdp,
                        pcsCandidates,
                        std::move(c),
                        [](const Candidate& c) -> const QValue& { return c.root; },
                        false,
                        any_in_budget);
                }
            }
            //
            vector<unique_ptr<Policy>> curr_policies;
            // Find and add unique, pruned combo-policies
            for (auto &c : pcsCandidates) {
                auto pi = make_unique<Policy>(mdp, stateIdx);
                pi->MergePolicies(c.children, *c.successors, mdp);
                pi->AddAction((int)stateIdx, c.action_idx);
                pi->worth[(int)stateIdx] = c.root;

                auto &tab = piTable[1-prevPolicies][currStateSetIdx];
                auto [pi_it, inserted] = tab.insert(std::move(pi));
                if (!inserted) {
                    pi_it->get()->included_state_actions.emplace_back(stateIdx, c.action_idx);
                }
            }
            currStateSetIdx++;
        }

        // Construct final policy vector
        auto &solns = piTable[1 - prevPolicies][0];
        result.reserve(solns.size());
        for (auto it = solns.begin(); it != solns.end(); ) {
            auto policynh = solns.extract(it++);
            result.push_back(std::move(policynh.value()));
            histories.emplace_back();
            histories.back().reserve(result.back()->history_set.size());
            auto &hs = result.back()->history_set;
            for (auto hist_it = hs.begin(); hist_it != hs.end(); ) {
                auto histnh = hs.extract(hist_it++);
                histories.back().emplace_back(std::move(histnh.value()));
            }
            result.back()->history_set.clear();
        }
    }

    template <typename Callback>
    void EnumeratePolicyCombos(const std::vector<const policy_set*>& choices, std::size_t depth, std::vector<const Policy*>& combination, Callback&& callback) {
        if (depth == choices.size()) {
            callback(combination);
            return;
        }
        for (const auto& policy : *choices[depth]) {
            combination[depth] = policy.get();
            EnumeratePolicyCombos(choices, depth + 1, combination, callback);
        }
    }
    
    vector<vector<Policy*>> GetSuccessorPolicyCombos(vector<Successor*>* successors, vector<policy_set>& piTable, vector<size_t>& piStateLookup) {
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
                //mdp.heuristicQValue(qval, *mdp.states[target]);
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
    void gatherQValue(QValue& new_qv, vector<Successor*>* successors, vector<Policy*>& combo, int currentTime) {
        std::vector<QValue*> baselines_ = std::vector<QValue*>(successors->size());
        for (int scrIdx=0; scrIdx < successors->size(); ++scrIdx) {
            baselines_[scrIdx] = &combo[scrIdx]->worth[(*successors)[scrIdx]->target];
        }
        new_qv = mdp.MultiGather(*successors, baselines_);
}
};

