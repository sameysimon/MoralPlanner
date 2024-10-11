//
// Created by Simon Kolker on 30/08/2024.
//

#include "MEHR.hpp"

#include <iostream>

vector<double long>* MEHR::findNonAccept(vector<QValue>& expectedWorths, vector<shared_ptr<vector<QValue>>>& outcomeWorths, vector<shared_ptr<vector<double long>>>& probability) {
    vector<double long>* non_accept = new vector<double long>(expectedWorths.size(), 0);
    int potential_attacks = 0;
    int r = 0;
    vector<int*> expected_edges = vector<int*>(); // Stores [x,y,] if policy x defeats policy y in expectation.
    vector<vector<int*>> attackedOutcomes = vector<vector<int*>>(); // Stores for each solution, [oIdx,tIdx] if attacked on outcome oIdx by theory tIdx.
    for (int i = 0; i < expectedWorths.size(); i++) {
        attackedOutcomes.push_back(vector<int*>());
    }

    for (int oneIdx=0; oneIdx<expectedWorths.size(); ++oneIdx) {
        QValue& oneExp = expectedWorths[oneIdx];
        for (int twoIdx=oneIdx+1; twoIdx<expectedWorths.size(); ++twoIdx) {
            std::cout << oneIdx << " and " << twoIdx << std::endl;

            QValue& twoExp = expectedWorths[twoIdx];
            vector<int> attackingTheories = vector<int>();
            vector<int> reverseTheories = vector<int>();
            r = mdp.compareExpectations(oneExp, twoExp, attackingTheories, reverseTheories);

            if (r == 0) { continue; }

            if (r==1 or r==2) {
                std::cout << "Solution: " << oneIdx << " E("<< expectedWorths[oneIdx].toString() << ") attacks Solution: " << twoIdx << " E("<< expectedWorths[twoIdx].toString() << ")" << std::endl;
                checkForAttack(oneIdx, twoIdx, outcomeWorths, probability, attackedOutcomes, non_accept, attackingTheories);
            }
            if (r==-1 or r==2) {
                std::cout << "Solution: " << twoIdx << " E("<< expectedWorths[twoIdx].toString() << ") attacks Solution: " << oneIdx << " E("<< expectedWorths[oneIdx].toString() << ")" << std::endl;
                checkForAttack(twoIdx, oneIdx, outcomeWorths, probability, attackedOutcomes, non_accept, reverseTheories);
            }
        }
    }
    /*for (int* edge: expected_edges) {
        int attPolicyIdx = edge[0];
        int defPolicyIdx = edge[1];
        shared_ptr<vector<QValue>> attackerWorths = outcomeWorths[attPolicyIdx];
        shared_ptr<vector<QValue>> defenderWorths = outcomeWorths[defPolicyIdx];
        shared_ptr<vector<double>> defenderProbs = probability[defPolicyIdx];
        std::cout << "Potential Attack from Sol_id " << attPolicyIdx << " TO Sol_ID " << defPolicyIdx << std::endl;
        std::cout << "  Attacker SolID " << attPolicyIdx << " expected worth: " << expectedWorths[attPolicyIdx].toString() << std::endl;
        std::cout << "  Victim SolID " << defPolicyIdx << " expected worth: " << expectedWorths[defPolicyIdx].toString() << std::endl;

        for (int attIdx = 0; attIdx < attackerWorths->size(); ++attIdx) {
            for (int defIdx = 0; defIdx < defenderWorths->size(); ++defIdx) {
                std::cout << "      Can  " << attackerWorths->at(attIdx).toString() << " beat " << defenderWorths->at(defIdx).toString() << " ("<< defenderProbs->at(defIdx) << ") ?" << std::endl;
                attacks = mdp.countAttacks(attackerWorths->at(attIdx), defenderWorths->at(defIdx));
                (*non_accept)[defPolicyIdx] += attacks * defenderProbs->at(defIdx);
                if (attacks > 0) {
                    std::cout << "          = " << attacks << " * " << defenderProbs->at(defIdx) << " = " << attacks * defenderProbs->at(defIdx) << " non-acceptability." << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }*/
    std::cout << std::endl << std::endl;
    for (int i = 0; i < non_accept->size(); ++i) {
        std::cout << "Solution " << i << " non-accept = " << (*non_accept)[i] << std::endl;
    }
    return non_accept;
}



void MEHR::checkForAttack(int sourceSol, int targetSol, vector<shared_ptr<vector<QValue>>>& outcomeWorths, vector<shared_ptr<vector<double long>>>& probability, vector<vector<int*>>& attackedOutcomes, vector<double long>* non_accept, vector<int>& theories) {
    bool ignore=false;
    for (int attIdx = 0; attIdx < outcomeWorths[sourceSol]->size(); ++attIdx) {
        for (int defIdx = 0; defIdx < outcomeWorths[targetSol]->size(); ++defIdx) {
            for (int tIdx : theories) {
                if (tIdx==mdp.non_moralTheoryIdx) { continue;}
                // Check if defender already attacked.
                for (auto elem : attackedOutcomes[targetSol]) {
                    if (elem[0]==tIdx && elem[1]==defIdx) { ignore=true; break; }
                }
                if (ignore) {continue;}
                if (mdp.theories[tIdx]->attack(outcomeWorths[sourceSol]->at(attIdx),  outcomeWorths[targetSol]->at(defIdx))==1) {
                    // Store this attack.
                    auto newAttack = new int[2];
                    newAttack[0] = tIdx;
                    newAttack[1] = defIdx;
                    attackedOutcomes[targetSol].push_back(newAttack);
                    // Add to non-acceptability.
                    (*non_accept)[targetSol] += probability[targetSol]->at(defIdx);
                    std::cout << "     Theory Id " << tIdx << ". Outcome with " << outcomeWorths[sourceSol]->at(attIdx).toString() << " attacks outcome with" << outcomeWorths[targetSol]->at(defIdx).toString() << " for Prob +" << probability[targetSol]->at(defIdx) << " = " << (*non_accept)[targetSol] << std::endl;
                }

            }
        }
    }

}

