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


void MDP::outNout() {
    std::cout << "nout" << std::endl;
}

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

/**
 * Progresses down ranks, searching for earliest inequality.
 * @param useRanks whether to use Weak-Lexicographic ranks.
 * @return 1 for forward-attack, -1 for back-attack, 0 for draw.
 */
int MDP::compareQValues(QValue& qv1, QValue& qv2, bool useRanks) {
    int r = 0; // 0 is draw, 1 is forward-attack, -1 is back-attack, 2 is dilemma
    int newR = 0;
    for (std::vector<int>& theoriesInRank : groupedTheoryIndices) {
        for (int theoryIdx : theoriesInRank) {
            // Compare theory.
            newR = qv1.expectations[theoryIdx]->compare(*qv2.expectations[theoryIdx]);
            if (r==0)
                r = newR;
            else if (r!=newR and (r!=0 and newR!=0))
                return 2;// Disagreement on the same level, thus draw.
        }
        if (r!=0 and useRanks) {
            return r;// Decision on this highest level, return attack.
        }
    }
    return r;
}

/**
 * Counts the number of moral theories where qv1 attacks qv2.
 * @return number of attacks.
 */
int MDP::countAttacks(QValue& qv1, QValue& qv2) {
    int attacks = 0;
    for (int i=0; i<theories.size(); ++i) {
        // TODO change to a broader potentially multi-theory comparison thing.
        if (qv1.expectations[i]->compare(*qv2.expectations[i])==1) {
            attacks++;
        }
    }
    return attacks;
}

bool MDP::checkInBudget(QValue& qval) {
    if (non_moralTheoryIdx==-1) { return true; }
    ExpectedUtility* eu = static_cast<ExpectedUtility*>(qval.expectations[non_moralTheoryIdx]);
    if (eu->value > budget) {
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

