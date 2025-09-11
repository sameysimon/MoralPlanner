//
// Created by Simon Kolker on 23/10/2024.
//

#ifndef POLICY_HPP
#define POLICY_HPP
#include "MoralTheory.hpp"
#include "MDP.hpp"
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
    Policy() {
        policy = unordered_map<int, int>();
        worth = unordered_map<int, QValue>();
    }
    Policy(Policy& pi) : Policy() {
        importPolicy(pi);
    }
    void importPolicy(Policy& pi) {
        for (auto it : pi.policy) {
            policy[it.first] = it.second;
        }
        worth.insert(pi.worth.begin(), pi.worth.end());
    }
    void setAction(int state, int stateTimeAction) {
        policy[state] = stateTimeAction;
    }
    void setWorth(int state, QValue& qval) {
        worth[state] = qval;
    }
    WorthBase* getWorthAtTheory(int time, int stateIdx, int theoryIdx) {
        if (worth.find(stateIdx) != worth.end()) {
            return worth[stateIdx].expectations[theoryIdx];
        }
        throw runtime_error("ME ERROR--Policy does not exist at time " + to_string(time) + " , state " + to_string(stateIdx));
    }
    [[nodiscard]] string toString(MDP& mdp) const {
        std::stringstream ss;
        ss << "Policy { ";
        vector<int> epochOrder = vector<int>();
        for (auto it : policy) {
            epochOrder.push_back(it.first);
        }
        sort(epochOrder.begin(), epochOrder.end());//TODO will this work.
        reverse(epochOrder.begin(), epochOrder.end());
        for (auto sIdx : epochOrder) {
            int time = mdp.states[sIdx]->time;
            int stateAction = policy.at(sIdx);
            ss << "(" << time << ","<< sIdx << ") ->" << (*mdp.getActions(*mdp.states[sIdx]))[stateAction]->label << "{" << worth.at(sIdx).toString() << "}"<< ";  ";
        }
        ss << "}";
        return ss.str();
    }
    string getActionAsString(MDP& mdp, int state) {
        return mdp.getActions(*mdp.states[state])->at(policy.at(state))->label;
    }
};

struct PolicyPtrEqual {
    bool operator()(const Policy* lhs, const Policy* rhs) const {
        return lhs->policy == rhs->policy && lhs->worth == rhs->worth;
    }
};
struct PolicyPtrHash {
    size_t operator()(const Policy* pi) const {
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

#endif //POLICY_HPP
