//
// Created by Simon Kolker on 23/10/2024.
//
#pragma once
#include "MoralTheory.hpp"
#include "MDP.hpp"
#include "QValue.hpp"
#include <functional>
#include <unordered_set>

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
    QValue* ptr_policy_value = nullptr;

    explicit Policy(MDP& mdp) {
        policy = unordered_map<int, int>();
        worth = unordered_map<int, QValue>();
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
        /*for (auto i = 0; i < policy.size(); i++) {
            policy[i] = pi.getAction(i);
            worth[i] = pi.worth[i];
        }*/
        for (auto it : pi.policy) {
            policy[it.first] = it.second;
        }
        worth.insert(pi.worth.begin(), pi.worth.end());
    }

    int getAction(int state) {
        if (policy.find(state)==policy.end()) {
            return -1;
        }
        return policy[state];
    }
    void setAction(int state, int stateTimeAction) {
        policy[state] = stateTimeAction;
    }
    QValue& AddWorth(MDP& mdp, int state) {
        if (worth.find(state)==worth.end()) {
            auto qv = QValue(mdp);
            worth.emplace(state, qv);
        }
        return worth[state];
    }
    QValue* getExpectationPtr() {
        if (ptr_policy_value==nullptr) {
            ptr_policy_value = &worth[0];
        }
        return ptr_policy_value;
    }
    WorthBase* getWorthAtTheory(int time, int stateIdx, int theoryIdx) {
        if (worth.find(stateIdx) != worth.end()) {
            return worth[stateIdx].expectations[theoryIdx].get();
        }
        throw runtime_error("ME ERROR--Policy does not exist at time " + to_string(time) + " , state " + to_string(stateIdx));
    }
    [[nodiscard]] string toString(MDP& mdp) const {
        std::stringstream ss;
        ss << "Policy { ";
        vector<int> epochOrder = vector<int>();
        sort(epochOrder.begin(), epochOrder.end());//TODO will this work.
        reverse(epochOrder.begin(), epochOrder.end());
        for (auto i = 0; i < policy.size(); i++) {
            int time = mdp.states[i]->time;
            int stateAction = policy.at(i);
            ss << "(" << time << ","<< i << ") ->" << (*mdp.getActions(*mdp.states[i]))[stateAction]->label << "{" << worth.at(i).toString() << "}"<< ";  ";
        }
        ss << "}";
        return ss.str();
    }
    string getActionAsString(MDP& mdp, int state) {
        return mdp.getActions(*mdp.states[state])->at(policy.at(state))->label;
    }
};

struct PolicyPtrEqual {
    bool operator()(const unique_ptr<Policy>& lhs, const unique_ptr<Policy>& rhs) const {
        return lhs->policy == rhs->policy && lhs->worth == rhs->worth;
    }
};
struct PolicyPtrHash {
    size_t operator()(const unique_ptr<Policy>& pi) const {
        QValueHash qvalHasher;
        size_t hash = 0;
        // Combine hash for policy
        for (const auto& item : pi->policy) {
            size_t keyHash = std::hash<int>{}(item.first);
            size_t valHash = std::hash<int>{}(item.second);
            hash ^= keyHash ^ valHash;
        }
        // Combine hash for worth
        for (const auto& item : pi->worth) {
            size_t keyHash = std::hash<int>{}(item.first);
            size_t valHash = qvalHasher(item.second);
            hash ^= keyHash ^ valHash;
        }
        return hash;
    }
};

