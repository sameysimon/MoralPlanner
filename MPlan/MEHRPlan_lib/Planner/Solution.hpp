//
//  Solution.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#ifndef Solution_hpp
#define Solution_hpp

#include "MoralTheory.hpp"
#include "Utilitarianism.hpp"
#include "MDP.hpp"
#include "Expecter.hpp"
#include "QValue.hpp"
#include <iostream>

class Solution {
    MDP* mdp;

    bool checkForDominance(const Solution& other) {
        if (mdp != other.mdp)
            throw std::runtime_error("Solution::operator>: Solutions defined for different MDPs. Something very wrong has happened.");
        if (expecters.size() != other.expecters.size())
            throw std::runtime_error("Solution::operator>: Solution expecters have different sizes. Something very wrong has happened.");

        bool atLeastOneGreater=false;
        // TODO more expensive than it probably needs to be.
        for (int i = 0; i < expecters.size(); i++) {
            // If beaten anywhere, then this Sol doesn't dominate other.
            if (*other.expecters[i] > *expecters[i]) {
                return false;
            }
            // Only need to check if wins on this theory if not already found one.
            if (not atLeastOneGreater) {
                if (*other.expecters[i] < *expecters[i])
                    atLeastOneGreater=true;
            }
        }
        // If reached here, not beaten on any theory
        // Only dominates if there was at least one theory where this is pref'd.
        return atLeastOneGreater;
    }

public:
    std::vector<Expecter*> expecters;
    std::vector<std::vector<int>> policy;
    std::vector<std::vector<int>> updateIndex;
    // Create empty Solution (hanging pointers)
    Solution() {}
    // Creates an expectation for each theory in the MDP.
    Solution(MDP &_mdp, bool useHeuristics=false) {
        this->mdp = &_mdp;
        this->expecters = std::vector<Expecter*>();
        this->policy = std::vector<std::vector<int>>(mdp->horizon+1);
        this->updateIndex = std::vector<std::vector<int>>(mdp->horizon+1);
        // Initialise empty expecters.
        for (auto& m : _mdp.theories) {
            // Make expecter for the moral theory
            Expecter* e;
            if (useHeuristics) {
                e = m->makeHeuristicExpecter(mdp->states, mdp->horizon);
            } else {
                e = m->makeExpecter(mdp->total_states, mdp->horizon);
            }
            // Add it to the list.
            this->expecters.push_back(e);
        }
        // Initialise empty policy
        for (int t =0; t < _mdp.horizon; ++t) {
            this->policy[t] = std::vector<int>(mdp->states.size(), -1);
            this->updateIndex[t] = std::vector<int>(mdp->states.size(), -1);
        }
    }
    // Clones by value an existing Solution (including its expecters and policy.).
    Solution(const Solution &other) {
        this->expecters = std::vector<Expecter*>(other.expecters.size());
        mdp = other.mdp;
        for (int i=0; i<other.expecters.size(); ++i) {
            this->expecters[i] = other.expecters[i]->clone();
        }
        policy = other.policy;
        updateIndex = other.updateIndex;
    }
    Solution* clone() {
        return new Solution(*this);
    }
    ~Solution() {
        for (auto exp: expecters) {
            delete exp;
        }
    }
    Solution& operator=(const Solution& other) {
        if (this == &other) {
            return *this;
        }
        mdp = other.mdp;
        if (other.expecters.size() != expecters.size()) {
            throw("Solution::operator=() Expecter sizes are not equal. Something very wrong has happened.");
        }
        for (int i=0; i<other.expecters.size(); ++i) {
            *expecters[i] = *(other.expecters[i]);
        }

        return *this;
    }
    // Pareto dominance operator. Throws exception if Solution have different MDPs.
    bool operator<(const Solution& other) { throw std::runtime_error("Solution::operator<: Less than not implemented. Use greater than."); }
    // Pareto dominance operator. Throws exception if Solution have different MDPs.
    // Dominates if preferred by at least one objective and there are no objectives where not preferred.
    bool operator>(const Solution& other) { return checkForDominance(other); }


    // True if equivalent to argument sol.
    bool equivalencyCheck(Solution &other) {
        for (int i=0; i<expecters.size(); i++) {
            if (not expecters[i]->isEquivalent(*other.expecters[i])) {
                return false;
            }
        }
        return true;
    }
    //
    // SETTERS...
    //
    void setToQValue(State& state, int time, QValue& qval) {
        for (int i = 0; i < qval.expectations.size(); ++i) {
            expecters[i]->setToValue(state, time, qval.expectations[i]);
        }
    }
    void setAction(State& state, int time, int stateActionID) {
        policy[time][state.id] = stateActionID;
    }

    //
    // GETTERS AND GATHERERS
    //
    QValue vectorGather(std::vector<Successor*>& successors, int time) {
        QValue qval = QValue();
        for (auto exp : expecters) {
            WorthBase* wb = exp->gather(successors, time);//TODO stop from copying here.
            qval.addToExpectations(wb);
        }
        return qval;
    };
    void loadQValue(State& state, int time, QValue& qval) {
        for (auto exp : expecters) {
            WorthBase* wb = exp->expectations[time][state.id];
            qval.addToExpectations(wb);
        }
    }
    void cautiousCopyAtTime(int time, Solution& oth) {
        if (time > mdp->horizon-1) { return; }
        for (int stateIdx = 0; stateIdx < mdp->states.size(); stateIdx++) {
            if (updateIndex[time][stateIdx] < oth.updateIndex[time][stateIdx]) {
                // This solution behind other solution.
                for (int expIdx = 0; expIdx < expecters.size(); expIdx++) {

                    expecters[expIdx]->expectations[time][stateIdx] = oth.expecters[expIdx]->expectations[time][stateIdx]->clone();
                    policy[time][stateIdx] = oth.policy[time][stateIdx];
                    // Make the copy.
                }
                updateIndex[time][stateIdx] = oth.updateIndex[time][stateIdx];
            }
        }
    }

    std::string policyToString();
    std::string worthToString();

};

struct SolutionHash {
    size_t operator()(const std::shared_ptr<Solution> sol) const {
        std::size_t hashValue = 0;
        std::hash<int> hashFn;
        int horizon = sol->expecters[0]->expectations.size();
        int num_states = sol->expecters[0]->expectations[0].size();
        for (int t = 0; t < horizon-1; ++t) {
            for (int s = 0; s < num_states; ++s) {
                hashValue ^= hashFn(sol->policy[t][s]) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            }
        }
        return hashValue;
    }
};

struct SolutionEqual {
    bool operator()(const std::shared_ptr<Solution> lhs, const std::shared_ptr<Solution> rhs) const {
        int horizon = lhs->expecters[0]->expectations.size();
        int num_states = lhs->expecters[0]->expectations[0].size();
        for (int t = 0; t < horizon-1; ++t) {
            for (int s = 0; s < num_states; ++s) {
                if (lhs->policy[t][s] != rhs->policy[t][s]) {
                    return false;
                }
            }
        }
        return true;
    }
};

struct InsertOrderCompare {
    bool operator()(const std::shared_ptr<Solution> lhs, const std::shared_ptr<Solution> rhs) const {
        return true;
    }
};
#endif /* Solution_hpp */