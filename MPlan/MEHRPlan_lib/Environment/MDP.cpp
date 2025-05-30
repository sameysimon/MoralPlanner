//
//  MDP.cpp
//  MPlan
//
//  Created by e56834sk on 10/07/2024.
//


#include <iostream>
#include <vector>
#include "MDP.hpp"
#include "Utilitarianism.hpp"

#include <vector>


void MDP::getNoBaseLineQValue(State& state, int stateActionIndex, QValue& qval) {
    blankQValue(qval);
    auto scrs = getActionSuccessors(state, stateActionIndex);
    if (scrs->empty()) { return; }
    std::vector<WorthBase*> nullBaselines = std::vector<WorthBase*>(scrs->size(), nullptr);
    for (auto & t : considerations) {
        std::fill(nullBaselines.begin(), nullBaselines.end(), t->newWorth());
        qval.expectations.push_back(t->gather(*scrs, nullBaselines, false));
        delete nullBaselines[0];
    }
}

void MDP::addCertainSuccessorToQValue(QValue& qval, Successor* scr) {
    std::vector<Successor*> successors = {scr};
    auto baselines = std::vector<WorthBase*>(1);
    for (int i=0; i<considerations.size(); ++i) {
        baselines[0]=qval.expectations[i];
        qval.expectations[i] = considerations[i]->gather(successors, baselines, true);
    }
}
void MDP::blankQValue(QValue& qval) {
    for (auto & t : considerations) {
        qval.expectations.push_back(t->newWorth());
    }
}

int MDP::compareQValueByRank(QValue& qv1, QValue& qv2, int rank) {
    std::vector<size_t>& theoriesInRank = groupedTheoryIndices[rank];
    bool atLeastOneGreater = false;
    bool atLeastOneLesser = false;
    for (auto i : theoriesInRank) {
        int newR = qv1.expectations[i]->compare(*qv2.expectations[i]);
        if (newR==-1) {
            atLeastOneGreater = true;
        } else if (newR==1) {
            atLeastOneLesser = true;
        }
    }
    // Reverse attack
    if (atLeastOneGreater and not atLeastOneLesser) {
        return -1;
    }
    // Forward Attack
    if (not atLeastOneGreater and atLeastOneLesser) {
        return 1;
    }
    // Dilemma
    if (atLeastOneGreater and atLeastOneLesser) {
        return 2;
    }
    return 0;
}

/**
 * Compares QValues by considerations only. Meant for planning. Does not use lexicographic order.
 * @return 1 for qv1 attacks qv2 (forward-attack), -1 for reverse, 0 for draw.
 */
int MDP::CompareByConsiderations(QValue& qv1, QValue& qv2) {
    int result = 0;
    int currResult = 0;
    for (auto pCon : considerations) {
        currResult = qv1.expectations[pCon->id]->compare(*qv2.expectations[pCon->id]);
        if (result != 0 && currResult != result) {
            // Disagreement with last result. Attakcs fires both ways. Draw.
            return 0;
        }
        result = currResult;
    }
    return result;
}



/**
 * Progresses down ranks, searching for earliest inequality.
 * @param useRanks whether to use Weak-Lexicographic ranks.
 * @return 1 for forward-attack, -1 for back-attack, 0 for draw, 2 for draw.
 */
int MDP::CompareByTheories(QValue& qv1, QValue& qv2, bool useRanks) {
    int result = 0;
    int newResult = 0;
    for (int rank=0; rank<groupedTheoryIndices.size(); rank++) {
        newResult = compareQValueByRank(qv1, qv2, rank);
        if (!useRanks and result != 0 and newResult != result and newResult != 0) {
            return 0; // Disagreement between theories.
        }
        if (useRanks and newResult != 0) {
            return newResult;
        }
        if (newResult != 0) {result = newResult;}
    }
    if (!useRanks and this->non_moralTheoryIdx!=-1) {
        // Use cost theory.
        newResult = qv1.expectations[non_moralTheoryIdx]->compare(*qv2.expectations[non_moralTheoryIdx]);
        if (result!=0 and newResult!=result and newResult!=0) {
            return 0;
        }
        if (newResult != 0) {
            return newResult;
        }
    }
    return result;
}

/**
 * Finds attack direction and theories used in attacks. Sensitive to Weak Lexicographic Order!
 * @param forwardTheories Fills with indices of theories in mdp.theories s.t. qv1 to attack qv2.
 * @param reverseTheories Fills with indices of theories in mdp.theories s.t. qv2 to attack qv1.
 * @return 1 if qv1 attacks qv2; -1 if qv2 attacks qv1; 0 if no attacks; 2 if both attack each other.
 */
int MDP::compareExpectations(QValue& qv1, QValue& qv2, std::vector<int>& forwardTheories, std::vector<int>& reverseTheories) {
    // TODO Make sensitive to multi-consideration theories.

    // Find rank where there is a decision.
    int attackDirection = 0;// Initially 0, means undecided.
    for (int rank=0; rank<groupedTheoryIndices.size(); rank++) {
        std::vector<size_t>& theoriesInRank = groupedTheoryIndices[rank];
        int rankResult = 0;
        for (auto tIdx : theoriesInRank) {
            int newR = mehr_theories[tIdx]->CriticalQuestionTwo(qv1, qv2);
            // Still searching for direction
            if (newR!=rankResult && rankResult!=0 && attackDirection==0) {
                // Disagreement at rank, but no attackDirection established.
                attackDirection=2;//Dilemma.
            }
            if (newR==1 || newR==2) {
                forwardTheories.push_back((ushort)tIdx);
            }
            if (newR==-1 || newR==2) {
                reverseTheories.push_back((ushort)tIdx);
            }
            rankResult = newR;
        }
        if (attackDirection==0) {
            attackDirection=rankResult;// If a direction has been established then use it.
        }
    }
    // Depending on the direction of attack.
    if (attackDirection==1) {
        reverseTheories.clear();
    }
    if (attackDirection==-1) {
        forwardTheories.clear();
    }
    return attackDirection;
}

/**
 * Counts the number of moral theories where qv1 attacks qv2.
 * // TODO change to a broader potentially multi-theory comparison thing.
 * @return number of attacks.

int MDP::countAttacks(QValue& qv1, QValue& qv2) {
    int attacks = 0;
    for (int i=0; i<theories.size(); ++i) {
        if (this->theories[i]->attack(qv1, qv2)==1) {
            attacks++;
            std::cout << "        Theory ID " << i <<  " attack." << std::endl;
        }

    }
    return attacks;
}
*/


/**
 * Sets a QValue expectations field with heuristic estimates for state.
 * @param qval An initialised QValue to fill with heuristics
 * @param state The state whose value must be estimated.
 */
void MDP::heuristicQValue(QValue& qval, State& state) {
    for (int i=0; i<considerations.size(); ++i) {
        qval.expectations[i] = considerations[i]->newHeuristic(state);
    }
}

/**
 * If there is a non-moral cost theory, checks if qval is in budget.
 * If there is no cost theory, all QValues are in budget, so returns true.
 * @return true if in budget or no non-moral cost, false otherwise.
 */
bool MDP::isQValueInBudget(QValue& qval) const {
    if (non_moralTheoryIdx==-1) { return true; }
    auto cost = static_cast<ExpectedUtility*>(qval.expectations[non_moralTheoryIdx]);
    return cost->value > -1 * budget;
}


MDP::~MDP()
{
    // Clear all states off the heap (they will clear successors off the heap.)
    for (State* s : states) {
        delete s;
    }
    // Clear actions off the heap
    for (Action* a : actions) {
        delete a;
    }
    // Clear moral theories off the heap.
    for (Consideration* mt : considerations) {
        delete mt;
    }
}

