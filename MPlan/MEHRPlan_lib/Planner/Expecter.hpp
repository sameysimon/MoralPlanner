//
//  Expecter.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#ifndef Expecter_hpp
#define Expecter_hpp

#include "MoralTheory.hpp"
#include "State.hpp"
#include <vector>

class MDP;
class WorthBase;


class Expecter {
private:
    MoralTheory* moralTheory;
    bool checkForDominance(const Expecter& other, int oppositeDirection) {
        if (expectations.size() != other.expectations.size())
            throw std::runtime_error("Expecter::operator>: Expecter expectations have different sizes. Something very wrong has happened.");
        if (moralTheory!=other.moralTheory)
            throw std::runtime_error("Expecter::operator>: Moral theories are different. Something very wrong has happened.");


        for (int t = 0; t < other.expectations.size(); ++t) {
            for (int s = 0; s < other.expectations[t].size(); ++s) {
                if (expectations[t][s]->compare(*other.expectations[t][s]) == oppositeDirection) { return false; }
            }
        }
        return true;
    }
public:
    std::vector<std::vector<WorthBase*>> expectations;
    // Constructor
    Expecter(int num_of_states, int horizon, MoralTheory* theory) {
        moralTheory = theory;
        expectations = std::vector<std::vector<WorthBase*>>(horizon+1);// Store one extra for null values.
        for (auto& e : expectations) {
            e = std::vector<WorthBase*>(num_of_states);
        }
    }
    Expecter(const Expecter& other) {
        expectations = std::vector<std::vector<WorthBase*>>(other.expectations.size());
        moralTheory = other.moralTheory;
        for (int t =0; t < other.expectations.size(); ++t) {
            expectations[t] = std::vector<WorthBase*>(other.expectations[t].size());
            for (int s = 0; s < other.expectations[t].size(); ++s) {
                expectations[t][s] = other.moralTheory->newExpectation();
                *expectations[t][s] = *other.expectations[t][s];
            }
        }
    }
    Expecter* clone() {
        return new Expecter(*this);
    }
    ~Expecter() {
        for (int t = 0; t < expectations.size(); ++t) {
            for (int s = 0; s < expectations[t].size(); ++s) {
                delete expectations[t][s];
            }
        }
    }
    Expecter& operator=(const Expecter& other) {
        if (this == &other) {
            return *this;
        }
        if (moralTheory != other.moralTheory) {
            throw("Expecter::operator= Expectations have different moral theory. Something has gone wrong");
        }

        // Copy worth base for all time/states.
        for (int t =0; t < other.expectations.size(); ++t) {
            for (int s = 0; s < other.expectations[t].size(); ++s) {
                *expectations[t][s] = *other.expectations[t][s];
            }
        }
        return *this;
    }
    // Pareto dominance operator. Throws exception if Expecters have different moral theories.
    // True if for all time/state lhs is preferable to rhs.
    bool operator<(const Expecter& other) { return checkForDominance(other, 1); }
    // Pareto dominance operator. Throws exception if Expecters have different moral theories.
    // True if for all time/state lhs is preferable to rhs.
    bool operator>(const Expecter& other) { return checkForDominance(other, -1); }




    bool isEquivalent(Expecter& other) {
        if (this->moralTheory != other.moralTheory) {
            throw new std::runtime_error("Tried to check equivalence between expecters of different type. Likely an error");//return false;
        }
        // If states at any time/state are not equivalent, then no equivalence.
        for (int t = 0; t < expectations.size(); ++t) {
            for (int s = 0; s < expectations[t].size(); ++s) {
                if (not expectations[t][s]->isEquivalent(*other.expectations[t][s])) {
                    return false;
                }
            }
        }

        return true;
    }

    WorthBase* gather(std::vector<Successor*>& successors, int time) {
        std::vector<WorthBase*> reducedExps = std::vector<WorthBase*>(successors.size());
        for (int i = 0; i < successors.size(); i++)
            reducedExps[i] = expectations[time][successors[i]->target];
        return moralTheory->gather(successors, reducedExps);
    }

    void setToValue(State& s, int t, WorthBase* worth) {
        if (expectations[t][s.id] != nullptr)
            delete expectations[t][s.id];// maybe problematic. If memory issues, i'll just use shared_ptrs.
        expectations[t][s.id] = worth;
    }

    void setToHeuristic(State& s, int t) {
        //expectations[state.id] = *moralTheory->getHeuristic(state);
    }
};

#endif /* Expecter_hpp */
