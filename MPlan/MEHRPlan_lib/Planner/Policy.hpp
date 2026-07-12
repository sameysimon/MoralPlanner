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
    size_t root_stateIdx = 0;

    unordered_set<unique_ptr<History>, UHistoryPtrHash, UHistoryPtrEqual> history_set;
    vector<pair<size_t, size_t>> included_state_actions;

    QValue* ptr_policy_value = nullptr;
    size_t semantic_hash = 0;
    bool semantic_hash_valid = false;

    explicit Policy(MDP&, std::size_t rootStateIdx = 0)
        : root_stateIdx(rootStateIdx) {}

    Policy(Policy& other, MDP& mdp)
        : Policy(mdp, other.root_stateIdx) {
        importPolicy(other);
    }

    Policy& operator=(const Policy& other) {
        if (this != &other) {
            policy = other.policy;
            worth = other.worth;
            root_stateIdx = other.root_stateIdx;
            forced_actions = other.forced_actions;
            included_state_actions = other.included_state_actions;
            ptr_policy_value = nullptr;
            null_semantic_hash();
        }
        return *this;
    }
    void null_semantic_hash() {
        semantic_hash_valid = false;
        semantic_hash = 0;
    }
    size_t get_semantic_hash() {
        if (semantic_hash_valid) {
            return semantic_hash;
        }
        semantic_hash = CalculateSemanticHash();
        semantic_hash_valid = true;
        return semantic_hash;
    }

    void importPolicy(Policy& pi) {
        for (auto it : pi.policy) {
            policy[it.first] = it.second;
        }
        worth.insert(pi.worth.begin(), pi.worth.end());
    }

    void setWorth(size_t stateIdx, QValue qv) {
        worth.insert_or_assign(static_cast<int>(stateIdx), qv );
        if (stateIdx==root_stateIdx) {
            ptr_policy_value = nullptr;
        }
        null_semantic_hash();
    }
    QValue& AddWorth(MDP& mdp, size_t state) {
        auto [it, inserted] = worth.try_emplace(static_cast<int>(state), mdp);
        if (inserted) {
            if (state==root_stateIdx) {
                ptr_policy_value = nullptr;
            }
            null_semantic_hash();
        }
        return it->second;
    }
    void AddAction(int state, int stateTimeAction) {
        policy.insert_or_assign(state, stateTimeAction);
    }

    void MergePolicies(vector<Policy*>& policies, vector<Successor*>& successors, MDP& mdp) {
        for (auto & pi : policies) {
            importPolicy(*pi, successors, mdp);
        }
    }

    void importPolicy(Policy& incoming, vector<Successor*>& successors, MDP& mdp) {
        // Copy simple fields:
        policy.insert(incoming.policy.begin(), incoming.policy.end());
        worth.insert(incoming.worth.begin(), incoming.worth.end());
        forced_actions.insert(forced_actions.end(),
            forced_actions.begin(),
            forced_actions.end());
        included_state_actions.insert(included_state_actions.end(),
            incoming.included_state_actions.begin(),
            incoming.included_state_actions.end()
        );

        // Copy histories, add current successor:
        for (auto successor : successors) {
            if (incoming.root_stateIdx != successor->target) {
                continue;
            }
            for (auto &it : incoming.history_set) {
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

    static bool checkActionsCompatible(const std::unordered_map<int,int>& currActionMap, const Policy& newPolicy) {
        for (const auto& [state, action] : newPolicy.policy) {
            auto it = currActionMap.find(state);
            if (it != currActionMap.end() && it->second != action) {
                return false;
            }
        }
        return true;
    }
    static bool checkActionsCompatible(const vector<Policy*>& pols) {
        unordered_map<int, int> combinedActions;
        for (auto pi : pols) {
            for (const auto& [state, action] : pi->policy) {
                auto [it, inserted] = combinedActions.emplace(state,action);
                if (!inserted && it->second != action) {
                    return false;
                }
            }

        }
        return true;
    }

    optional<int> getAction(int state) {
        if (policy.find(state)==policy.end()) {
            return nullopt;
        }
        return policy[state];
    }

    QValue* getExpectationPtr() {
        if (ptr_policy_value==nullptr) {
            ptr_policy_value = &worth[static_cast<int>(root_stateIdx)];
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



    // String functions
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



    size_t CalculateSemanticHash() {
        QValueHash qValHash;
        size_t result = qValHash(*getExpectationPtr());

        std::vector<std::size_t> hist_hashes;
        for (auto &hist : history_set) {
            size_t hist_hash = qValHash(hist->mWorth);
            int rounded = (int)(hist->probability*1000000.0);
            QValue::hash_combine(hist_hash, std::hash<int>()(rounded));
            hist_hashes.push_back(hist_hash);
        }
        std::sort(hist_hashes.begin(), hist_hashes.end());
        QValue::hash_combine(result, hist_hashes.size());
        for (size_t it : hist_hashes) {
            QValue::hash_combine(result, it);
        }
        hist_hashes.clear();
        for (auto &it : forced_actions) {
            size_t acts_hash = std::hash<size_t>()(it.first);
            QValue::hash_combine(acts_hash, std::hash<size_t>()(it.second));
            hist_hashes.push_back(acts_hash);
        }
        std::sort(hist_hashes.begin(), hist_hashes.end());
        QValue::hash_combine(result, hist_hashes.size());
        for (size_t it : hist_hashes) {
            QValue::hash_combine(result, it);
        }
        return result;
    }
    size_t CalculateActionMappingHash() {
        std::vector<std::pair<int, int>> actions;
        actions.reserve(policy.size());
        for (const auto& entry : policy) {
            actions.emplace_back(entry);
        }

        std::sort(actions.begin(), actions.end());

        std::size_t result = actions.size();
        for (const auto& [state, action] : actions) {
            std::size_t entryHash = std::hash<int>{}(state);
            QValue::hash_combine(entryHash, std::hash<int>{}(action));
            QValue::hash_combine(result, entryHash);
        }
        return result;
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
        return pi->get_semantic_hash();

    }
};

