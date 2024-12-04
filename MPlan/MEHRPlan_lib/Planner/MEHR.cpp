//
// Created by Simon Kolker on 30/08/2024.
//
#include "MEHR.hpp"
#include <iostream>

vector<double>* MEHR::findNonAccept(vector<QValue>& policyWorths, vector<vector<History*>>& histories) {
    non_accept = new vector<double>(policyWorths.size(), 0);
    int r = 0;
    vector<int*> expected_edges = vector<int*>(); // Stores [x,y,] if policy x defeats policy y in expectation.
    attacksByTarget = new vector<vector<Attack>*>();
    for (int i = 0; i < policyWorths.size(); i++) {
        auto attacksOnPolicy = new vector<Attack>();
        attacksByTarget->push_back(attacksOnPolicy);
    }

    for (int oneIdx=0; oneIdx<policyWorths.size(); ++oneIdx) {
        QValue& oneExp = policyWorths[oneIdx];
        for (int twoIdx=oneIdx+1; twoIdx<policyWorths.size(); ++twoIdx) {
            QValue& twoExp = policyWorths[twoIdx];
            // Compare all policies
            // Store which way different theories may attack:
            vector<int> attackingTheories = vector<int>();// Theories where pi_one attacks pi_two
            vector<int> reverseTheories = vector<int>();// Theories where pi_two attacks pi_one
            // Compare expectations of Policy idxOne to Policy idxTwo:
            if (oneIdx==1 && twoIdx==4) {
                oneIdx = oneIdx;
            }
            r = mdp.compareExpectations(oneExp, twoExp, attackingTheories, reverseTheories);

            // No greater expectations -> no attacks can be formed.
            if (r == 0) { continue; }

            // Check for attacks from policy one to policy two by attackingTheories
            if (r==1 or r==2) {
                //std::cout << "Solution: " << oneIdx << " E("<< expectedWorths[oneIdx].toString() << ") attacks Solution: " << twoIdx << " E("<< expectedWorths[twoIdx].toString() << ")" << std::endl;
                checkForAttack(oneIdx, twoIdx, histories, attackingTheories);
            }
            // Check for attacks from policy two to policy one by reverseTheories
            if (r==-1 or r==2) {
                //std::cout << "Solution: " << twoIdx << " E("<< expectedWorths[twoIdx].toString() << ") attacks Solution: " << oneIdx << " E("<< expectedWorths[oneIdx].toString() << ")" << std::endl;
                checkForAttack(twoIdx, oneIdx, histories, reverseTheories);
            }
        }
    }
    return non_accept;
}



void MEHR::checkForAttack(int sourceSol, int targetSol, vector<vector<History*>>& histories, vector<int>& theories) {
    bool ignore=false;
    if (sourceSol == 4) {
        ignore = false;
    }
    for (int attIdx = 0; attIdx < histories.at(sourceSol).size(); ++attIdx) {
        for (int defIdx = 0; defIdx < histories.at(targetSol).size(); ++defIdx) {
            for (int tIdx : theories) {
                if (tIdx==mdp.non_moralTheoryIdx) { continue;}

                // Check if target Argument has already been attacked...
                auto potAttacks = (*attacksByTarget)[targetSol];

                for (Attack& att : *potAttacks) {
                    if (att.targetHistoryIdx==defIdx && att.theoryIdx==tIdx) {
                        ignore = true;
                        break;
                    }
                }
                // If target argument is already attacked, don't try and add to it.
                if (ignore) {continue;}

                int result = mdp.theories[tIdx]->attack(histories.at(sourceSol).at(attIdx)->worth, histories.at(targetSol).at(defIdx)->worth);
                if (result==1) {
                    // Store this attack.
                    Attack att = Attack(sourceSol, targetSol, tIdx, attIdx, defIdx);
                    potAttacks->push_back(att);
                    // Add to non-acceptability.
                    (*non_accept)[targetSol] += histories.at(targetSol).at(defIdx)->probability;
                    //std::cout << "     Theory Id " << tIdx << ". Outcome with " << histories->at(targetSol)->at(defIdx)->worth.toString() << " attacks outcome with" << histories->at(targetSol)->at(defIdx)->worth.toString() << " for Prob +" << histories->at(targetSol)->at(defIdx)->probability << " = " << (*non_accept)[targetSol] << std::endl;
                }
                //TODO optimise to add attack if result==-1
            }
        }
    }

}

