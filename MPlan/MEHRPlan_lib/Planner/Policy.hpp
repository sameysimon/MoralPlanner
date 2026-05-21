//
// Created by Simon Kolker on 23/10/2024.
//
#pragma once
#include "MoralTheory.hpp"
#include "History.hpp"
#include "MDP.hpp"
#include "QValue.hpp"
#include <unordered_set>
#include <iostream>


using namespace std;

struct ArrayHash {
    size_t operator()(const array<int, 2>& arr) const {
        return hash<int>()(arr[0]) ^ hash<int>()(arr[1]);
    }
};
struct ArrayCompare {
    bool operator()(const array<int, 2>& lhs, const array<int, 2>& rhs) const {
        if (lhs[0] != rhs[0]) {
            return lhs[0] > rhs[0]; // Sort in descending order by time
        }
        return lhs[1] > rhs[1]; // Otherwise sort by state.
    }
};
struct ArrayEqual {
    bool operator()(const array<int, 2>& lhs, const array<int, 2>& rhs) const noexcept {
        return lhs[0]==rhs[0] && lhs[1]==rhs[1];
    }
};



class Policy {
public:
    unordered_map<int, int> policy;
    unordered_map<int, QValue> worth;
    vector<pair<size_t, size_t>> forced_actions;
    QValue* ptr_policy_value = nullptr;
    unordered_set<unique_ptr<History>, UHistoryPtrHash, UHistoryPtrEqual> history_set;
    vector<pair<size_t, size_t>> included_state_actions;
    size_t root_stateIdx = 0;

    explicit Policy(MDP& mdp, size_t root_stateIdx_ = 0) {
        policy = unordered_map<int, int>();
        worth = unordered_map<int, QValue>();
        root_stateIdx = root_stateIdx_;
    }
    Policy(Policy& pi, MDP& mdp) : Policy(mdp) {
        importPolicy(pi);
    }
    Policy& operator=(const Policy& other) {
        if (this != &other) {
            policy = other.policy;
            worth = other.worth;
        }
        return *this;
    }
    void importPolicy(Policy& pi) {
        for (auto it : pi.policy) {
            policy[it.first] = it.second;
        }
        worth.insert(pi.worth.begin(), pi.worth.end());
    }
    void importPolicy(Policy& pi, vector<Successor*>& successors, MDP& mdp) {
        // Copy simple fields:
        policy.insert(pi.policy.begin(), pi.policy.end());
        worth.insert(pi.worth.begin(), pi.worth.end());
        forced_actions.insert(forced_actions.end(), pi.forced_actions.begin(), pi.forced_actions.end());

        included_state_actions.insert(included_state_actions.end(), pi.included_state_actions.begin(), pi.included_state_actions.end());
        // Copy histories, add current successor:
        for (auto successor : successors) {
            if (pi.root_stateIdx != successor->target) {
                continue;
            }
            for (auto &it : pi.history_set) {
                unique_ptr<History> h = make_unique<History>(*it);
                mdp.AggregateWithCertainSuccessor(h->mWorth, successor);
                h->probability *= successor->probability;
                auto [x, inserted] = history_set.insert(std::move(h));
                if (inserted) {
                    // Update path on history if it exists
                    if (it->hasPath) {
                        x->get()->path = make_unique<vector<size_t>>(it->path->size()+1, 0);
                        auto *path = x->get()->path.get();
                        for (size_t i = 0; i < it->path->size(); ++i) {
                            (*path)[i] = it->path->at(i);
                        }
                        (*path)[it->path->size()] = successor->target;
                    }
                } else {
                    // Increase probability of existing history
                    x->get()->probability += h->probability;
                    if (it->hasPath) {
                        auto *path = x->get()->path.get();
                        path->push_back(successor->target);
                    }
                }
            }
        }
    }

    void MergePolicies(vector<Policy*>& policies, vector<Successor*>& successors, MDP& mdp) {
        for (auto & pi : policies) {
            importPolicy(*pi, successors, mdp);
        }
    }

    optional<int> getAction(int state) {
        if (policy.find(state)==policy.end()) {
            return nullopt;
        }
        return policy[state];
    }
    void addAction(int state, int stateTimeAction) {
        policy[state] = stateTimeAction;
    }
    QValue& AddWorth(MDP& mdp, size_t state) {
        if (worth.find(state)==worth.end()) {
            auto qv = QValue(mdp);
            worth.emplace(state, qv);
        }
        return worth[state];
    }
    QValue* getExpectationPtr() {
        if (ptr_policy_value==nullptr) {
            ptr_policy_value = &worth[root_stateIdx];
        }
        return ptr_policy_value;
    }
    WorthBase* getWorthAtTheory(int time, int stateIdx, int theoryIdx) {
        if (worth.find(stateIdx) != worth.end()) {
            return worth[stateIdx].expectations[theoryIdx].get();
        }
        throw runtime_error("ME ERROR--Policy does not exist at time " + to_string(time) + " , state " + to_string(stateIdx));
    }


    [[nodiscard]] string toString() const {
        string x = "";
        for (auto &it : policy) {
            auto it_w = worth.find(it.first);
            string y = "??";
            if (it_w != worth.end()) { y = std::format("{}", it_w->second.toString()); }
            x += std::format("  STATE {} WORTH ({}) CHOOSES {}\n", it.first,y, it.second);
        }
        return x;
    }

    string getActionAsString(MDP& mdp, int state) const {
        if (auto acts = mdp.getActions(*mdp.states[state])) {
            return acts->at(policy.at(state))->label;
        }
        return "";
    }
    [[nodiscard]] string getActionAsString(MDP& mdp, vector<shared_ptr<Action>>& acts) const {
        if (acts.size()==1) {
            return acts.at(0)->label;
        }
        string s = "(";
        for (auto &it : acts) {
            s += it->label + ", ";
        }
        s.resize(s.size()-2);
        s += ")";
        return s;
    }
    [[nodiscard]] string getActionAsString(const vector<int>& acts) const {
        if (acts.size()==1) {
            return format("{}", acts.at(0));
        }
        string s = "(";
        for (auto &it : acts) {
            s += format("{}", it);
        }
        s.resize(s.size()-2);
        s += ")";
        return s;
    }
};

struct PolicyPtrEqual {
    bool operator()(const unique_ptr<Policy>& lhs, const unique_ptr<Policy>& rhs) const {
        if (lhs->history_set.size() != rhs->history_set.size()) return false;
        if (lhs->forced_actions.size() != rhs->forced_actions.size()) return false;
        if (!(*lhs->getExpectationPtr() == *rhs->getExpectationPtr())) return false;

        // If forced actions are the same between policies:
        for (auto &lhs_it : lhs->forced_actions) {
            bool found = false;
            for (auto & rhs_it : rhs->forced_actions) {
                if (lhs_it == rhs_it) {
                found = true;
                break;
                }
            }
            if (!found) {
                return false;
            }
        }

        // if every history has an equivalent, then they match.
        for (auto &lhsHist : lhs->history_set) {
            bool found = false;
            for (auto &rhsHist : rhs->history_set) {
                if (lhsHist->isEquivalent(*rhsHist)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }
};
struct PolicyPtrHash {
    size_t operator()(const unique_ptr<Policy>& pi) const {
        // Combine hash for policy
        size_t hash = 0;
        QValueHash qValHash;
        QValue::hash_combine(hash, qValHash(*pi->getExpectationPtr()));
        for (auto &hist : pi->history_set) {
            QValue::hash_combine(hash, qValHash(hist->mWorth));
            int rounded = (int)(hist->probability*1000.0);
            QValue::hash_combine(hash, std::hash<int>()(rounded));
        }
        for (auto &it : pi->forced_actions) {
            QValue::hash_combine(hash, std::hash<size_t>()(it.first));
            QValue::hash_combine(hash, std::hash<size_t>()(it.second));
        }
        return hash;
    }
};

