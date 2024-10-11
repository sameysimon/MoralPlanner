//
//  MDP.cpp
//  MPlan
//
//  Created by e56834sk on 10/07/2024.
//


#include <iostream>
#include <vector>
#include "MDP.hpp"
#include "Solution.hpp"

void MDP::addCertainSuccessorToQValue(QValue& qval, Successor* scr) {
    std::vector<Successor*> successors = {scr};
    auto baselines = std::vector<WorthBase*>(1);
    for (int i=0; i<theories.size(); ++i) {
        baselines[0]=qval.expectations[i];
        qval.expectations[i] = theories[i]->gather(successors, baselines, true);
    }
}
void MDP::blankQValue(QValue& qval) {
    for (int i=0; i<theories.size(); ++i) {
        qval.expectations.push_back(theories[i]->newWorth());
    }
}

int MDP::compareQValueByRank(QValue& qv1, QValue& qv2, int rank) {
    std::vector<int>& theoriesInRank = groupedTheoryIndices[rank];
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
    if (atLeastOneGreater and not atLeastOneLesser) {
        return -1;
    }
    if (not atLeastOneGreater and atLeastOneLesser) {
        return 1;
    }
    if (atLeastOneGreater and atLeastOneLesser) {
        return 2;
    }
    return 0;
}


/**
 * Progresses down ranks, searching for earliest inequality.
 * @param useRanks whether to use Weak-Lexicographic ranks.
 * @return 1 for forward-attack, -1 for back-attack, 0 for draw, 2 for draw.
 */
int MDP::compareQValues(QValue& qv1, QValue& qv2, bool useRanks) {
    int result = 0;
    int newResult = 0;
    for (int rank=0; rank<groupedTheoryIndices.size(); rank++) {
        newResult = compareQValueByRank(qv1, qv2, rank);
        if (!useRanks and result != 0 and newResult != result) {
            return 0; // Disagreement between theories.
        }
        if (useRanks and newResult != 0) {
            return newResult;
        }
        result = newResult;
    }
    if (!useRanks and this->non_moralTheoryIdx!=-1) {
        // Use cost theory.
        newResult = qv1.expectations[non_moralTheoryIdx]->compare(*qv2.expectations[non_moralTheoryIdx]);
        if (result!=0 and newResult!=result) {
            return 0;
        }
        if (newResult != 0) {
            return newResult;
        }
    }
    return result;
}

int MDP::compareExpectations(QValue& qv1, QValue& qv2, std::vector<int>& forwardTheories, std::vector<int>& reverseTheories) {
    // Find rank where there is a decision.
    int attackDirection = 0;// Initially 0, means undecided.
    for (int rank=0; rank<groupedTheoryIndices.size(); rank++) {
        std::vector<int>& theoriesInRank = groupedTheoryIndices[rank];
        int rankResult = 0;
        for (auto tIdx : theoriesInRank) {
            int newR = qv1.expectations[tIdx]->compare(*qv2.expectations[tIdx]);
            // Still searching for direction
            if (newR!=rankResult && rankResult!=0 && attackDirection==0) {
                // Disagreement at rank, but no attackDirection established.
                attackDirection=2;//Dilemma.
            }
            if (newR==1 || newR==2) {
                forwardTheories.push_back(tIdx);
            }
            if (newR==-1 || newR==2) {
                reverseTheories.push_back(tIdx);
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
 */
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

bool MDP::checkInBudget(QValue& qval) {
    if (non_moralTheoryIdx==-1) { return true; }
    ExpectedUtility* eu = static_cast<ExpectedUtility*>(qval.expectations[non_moralTheoryIdx]);
    if (eu->value > budget*-1) {
        return true;
    }
    return false;
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
    for (MoralTheory* mt : theories) {
        delete mt;
    }
}

