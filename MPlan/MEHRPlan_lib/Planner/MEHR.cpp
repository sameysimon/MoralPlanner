//
// Created by Simon Kolker on 30/08/2024.
//

#include "MEHR.hpp"

vector<double>* MEHR::findNonAccept(vector<QValue>& expectedWorths, vector<vector<QValue>*>& outcomeWorths, vector<vector<double>*>& probability) {
    vector<double>* non_accept = new vector<double>(expectedWorths.size(), 0);
    int potential_attacks = 0;
    int r = 0;
    vector<int*> expected_edges = vector<int*>(); // Stores [x,y] if policy x defeats policy y in expectation.

    for (int oneIdx=0; oneIdx<expectedWorths.size(); ++oneIdx) {
        QValue oneExp = expectedWorths[oneIdx];
        for (int twoIdx=oneIdx; twoIdx<expectedWorths.size(); ++twoIdx) {
            QValue twoExp = expectedWorths[twoIdx];
            r = mdp.compareQValues(oneExp, twoExp, true);

            if (r == 0) { continue; }
            // New attack
            expected_edges.push_back(new int[2]);

            if (r==1 or r==2) {
                int* l = expected_edges[potential_attacks];
                l[0] = oneIdx;
                l[1] = twoIdx;
            }
            if (r==2) {
                expected_edges.push_back( new int[2] );
                potential_attacks++;
            }
            if (r==-1 or r==2) {
                expected_edges[potential_attacks][0] = twoIdx;
                expected_edges[potential_attacks][1] = oneIdx;
            }
            potential_attacks++;
        }
    }
    int attacks;
    for (int* edge: expected_edges) {
        int attPolicyIdx = edge[0];
        int defPolicyIdx = edge[1];
        vector<QValue>* attackerWorths = outcomeWorths[attPolicyIdx];
        vector<QValue>* defenderWorths = outcomeWorths[defPolicyIdx];
        vector<double>*defenderProbs = probability[defPolicyIdx];

        for (int attIdx = 0; attIdx < attackerWorths->size(); ++attIdx) {
            for (int defIdx = 0; defIdx < defenderWorths->size(); ++defIdx) {
                attacks = mdp.countAttacks(attackerWorths->at(attIdx), defenderWorths->at(defIdx));
                (*non_accept)[defPolicyIdx] += attacks * defenderProbs->at(defIdx);
            }
        }
    }
    return non_accept;
}

