//
// Created by Simon Kolker on 23/10/2024.
//

#ifndef POLICY_HPP
#define POLICY_HPP
#include "MoralTheory.hpp"
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
    unordered_map<array<int, 2>, int, ArrayHash, ArrayEqual> policy;
    unordered_map<array<int, 2>, QValue, ArrayHash, ArrayEqual> worth;
    Policy() {
        policy = unordered_map<array<int, 2>, int, ArrayHash, ArrayEqual>();
        worth = unordered_map<array<int, 2>, QValue, ArrayHash, ArrayEqual>();
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
    void setAction(int time, int state, int stateTimeAction) {
        policy[{time, state}] = stateTimeAction;
    }
    void setWorth(int time, int state, QValue& qval) {
        worth[{time,state}] = qval;
    }
    WorthBase* getWorthAtTheory(int time, int stateIdx, int theoryIdx) {
        if (worth.find({time, stateIdx}) != worth.end()) {
            return worth[{time, stateIdx}].expectations[theoryIdx];
        }
        throw runtime_error("ME ERROR--Policy does not exist at time " + to_string(time) + " , state " + to_string(stateIdx));
    }
    [[nodiscard]] string toString(MDP& mdp) const {
        std::stringstream ss;
        ss << "Policy { ";
        vector<array<int,2>> epochOrder = vector<array<int,2>>();
        for (auto it : policy) {
            epochOrder.push_back(it.first);
        }
        sort(epochOrder.begin(), epochOrder.end(), ArrayCompare());
        reverse(epochOrder.begin(), epochOrder.end());
        for (auto epoch : epochOrder) {
            int time = epoch[0];
            int sIdx = epoch[1];
            int stateAction = policy.at({time, sIdx});
            ss << "(" << time << ","<< sIdx << ") ->" << *(*mdp.getActions(*mdp.states[sIdx],time))[stateAction]->label << "{" << worth.at(epoch).toString() << "}"<< ";  ";
        }
        ss << "}";
        return ss.str();
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
            size_t keyHash = ArrayHash{}(item.first); // hash of the key array<int, 2>
            size_t valHash = std::hash<int>{}(item.second); // hash of the int action
            hash ^= keyHash ^ valHash;
        }

        // Combine hash for worth
        for (const auto& item : pi->worth) {
            size_t keyHash = ArrayHash{}(item.first); // hash of the key array<int, 2>
            size_t valHash = qvalHasher(item.second); // hash of the QValue object
            hash ^= keyHash ^ valHash;
        }

        return hash;
    }
};

#endif //POLICY_HPP
