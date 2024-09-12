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


// Object for pointing to worth values in a solution/elsewhere (WorthBase*)
// Instantiated by calling vectorGather on a Solution. Holds
// Used to hold/compare/add state-time-action's worth/state's estimation easily.
class QValue {
public:
    std::vector<WorthBase*> expectations;
    QValue() {
        expectations = std::vector<WorthBase*>();
    }
    bool greaterThan(QValue& qval);
    void addToExpectations(WorthBase* ev) {
        expectations.push_back(ev);
    };
    std::string toString() {
        std::string result = "";
        for (WorthBase* ev : expectations) {
            result += ev->ToString() + "; ";
        }
        return result;
    }
};




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
    // Create empty Solution (hanging pointers)
    Solution() {}
    // Creates an expectation for each theory in the MDP.
    Solution(MDP &_mdp) {
        this->mdp = &_mdp;
        this->expecters = std::vector<Expecter*>();
        this->policy = std::vector<std::vector<int>>(mdp->horizon+1);
        // Initialise empty expecters.
        for (auto& m : _mdp.theories) {
            // Make expecter for the moral theory
            Expecter* e = m->makeExpecter(mdp->total_states, mdp->horizon);
            // Add it to the list.
            this->expecters.push_back(e);
        }
        // Initialise empty policy
        for (int t =0; t < _mdp.horizon; ++t) {
            this->policy[t] = std::vector<int>(mdp->states.size());
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
    void setToQValue(State& state, int time, QValue& qval) {
       for (int i = 0; i < qval.expectations.size(); ++i) {
           expecters[i]->setToValue(state, time, qval.expectations[i]);
       }
    }
    void setAction(State& state, int time, Action& a) {
        policy[time][state.id] = a.id;
    }
    void setToHeuristic(MDP &mdp, State& state, int time);
    void setAllToHeuristic(MDP &mdp);
    QValue vectorGather(std::vector<Successor*>& successors, int time) {
        QValue qval = QValue();
        for (auto exp : expecters) {
            WorthBase* wb = exp->gather(successors, time);//TODO stop from copying here.
            qval.addToExpectations(wb);
        }
        return qval;
    };

    std::size_t policyHash(int time_replace, int state_replace, int action_replace) {
        std::size_t hashValue = 0;
        std::hash<int> hashFn;
        for (int t = 0; t < policy.size()-1; ++t) {//TODO sort horizon issue out. Time step 5 is blank. Not sure if it should be.
            for (int s = 0; s < policy.size(); ++s) {
                if (time_replace==t and state_replace==s) {
                    hashValue ^= hashFn(action_replace) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2); // Combine hash values
                } else
                    hashValue ^= hashFn(policy[t][s]) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2); // Combine hash values
            }
        }
        return hashValue;
    }
    std::string policyToString();
    std::string worthToString();



};

#endif /* Solution_hpp */