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

/**
 * Progresses down ranks, searching for earliest inequality.
 * @return 1 for forward-attack, -1 for back-attack, 0 for draw.
 */
int MDP::compareQValues(QValue& qv1, QValue& qv2) {
    int r = 0; // 0 is draw, 1 is forward-attack, -1 is back-attack.
    int newR = 0;
    for (std::vector<int>& theoriesInRank : groupedTheoryIndices) {
        for (int theoryIdx : theoriesInRank) {
            // Compare theory.
            newR = qv1.expectations[theoryIdx]->compare(*qv2.expectations[theoryIdx]);
            if (r==0)
                r = newR;
            else if (r!=newR and (r!=0 and newR!=0))
                return 0;// Disagreement on the same level, thus draw.
        }
        if (r!=0) {
            return r;// Decision on this highest level, return attack.
        }
    }
    return r;
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

