//
// Created by Simon Kolker on 30/08/2024.
//
#include "MEHR.hpp"
#include <iostream>
#include <chrono>

vector<double>* MEHR::findNonAccept(vector<QValue>& policyWorths, vector<vector<History*>>& histories) {
#ifdef DEBUG
    int counter = 0;
    chrono::time_point<chrono::steady_clock> total_start;
    chrono::time_point<chrono::steady_clock> total_end;
    total_start = chrono::high_resolution_clock::now();
#endif // DEBUG
    non_accept = new vector<double>(policyWorths.size(), 0);// Stores non-acceptability of each policy.
    int r = 0;// Stores result
    std::cout << "There are " << policyWorths.size() << " policies" << std::endl;
    vector<int*> expected_edges = vector<int*>(); // Stores [x,y,] if policy x defeats policy y in expectation.
    // Stores policy to list of attacks on policy.
    attacksByTarget = new vector<vector<Attack>>();
    for (int i = 0; i < policyWorths.size(); i++) {
        vector<Attack> attacksOnPolicy = vector<Attack>();
        attacksByTarget->push_back(attacksOnPolicy);
    }

    // Sort histories for each history
    //sortHistories(histories);

    vector<int> attackingTheories = vector<int>();// Theories where pi_one attacks pi_two
    vector<int> reverseTheories = vector<int>();// Theories where pi_two attacks pi_one
    for (int oneIdx=0; oneIdx<policyWorths.size(); ++oneIdx) {
        QValue& oneExp = policyWorths[oneIdx];
        for (int twoIdx=oneIdx+1; twoIdx<policyWorths.size(); ++twoIdx) {
#ifdef DEBUG
            chrono::time_point<chrono::steady_clock> start1 = chrono::high_resolution_clock::now();
#endif
            // Values that will be used.
            attackingTheories.clear();
            reverseTheories.clear();
            QValue& twoExp = policyWorths[twoIdx];
            // Compare expectations of policy Idx 1 to policy Idx 2
            // Store which way different theories may attack:
            r = mdp.compareExpectations(oneExp, twoExp, attackingTheories, reverseTheories);
#ifdef DEBUG
            chrono::time_point<chrono::steady_clock> end1 = chrono::high_resolution_clock::now();
            total_comp_policy_exps += chrono::duration_cast<chrono::microseconds>(end1 - start1).count();
#endif // DEBUG
            // No greater expectations -> no attacks can be formed.
            if (r == 0) { continue; }
#ifdef DEBUG
            chrono::time_point<chrono::steady_clock> start2 = chrono::high_resolution_clock::now();
#endif
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
#ifdef DEBUG
            chrono::time_point<chrono::steady_clock> end2 = chrono::high_resolution_clock::now();
            hist_exps += chrono::duration_cast<chrono::microseconds>(end2 - start2).count();
#endif

        }
    }
#ifdef DEBUG
    total_end = chrono::high_resolution_clock::now();
    double total = chrono::duration_cast<chrono::microseconds>(total_end-total_start).count();
    cout << "Total time: " << total << endl;
    cout << "Search time: " << search_time << " -- " << (search_time / total)*100 << "%" << endl;
    cout << "Attack time: " << attack_time << " -- " << (attack_time / total)*100 << "%" << endl;
    cout << "Total CFA time: " << total_CFA << " -- " << (total_CFA / total)*100 << "%" << endl << endl;

    cout << "Check actions time: " << total_comp_policy_exps << " -- " << (total_comp_policy_exps / total)*100 << "%" << endl;
    cout << "Big Check for attack time: " << hist_exps << " -- " << (hist_exps / total)*100 << "%" << endl;
    cout << "Clear time: " << clear_time << " -- " << (clear_time / total)*100 << "%" << endl;
    cout << "Remaining in big loop:" << ((total - clear_time - hist_exps - total_comp_policy_exps) / total)*100 << " %" << endl;
#endif
    return non_accept;
}



void MEHR::checkForAttack(int sourceSol, int targetSol, vector<vector<History*>>& histories, vector<int>& theories) {
#ifdef DEBUG    
    auto totalCFAStart = chrono::high_resolution_clock::now();
#endif
    bool ignore=false;
    for (int attIdx = 0; attIdx < histories.at(sourceSol).size(); ++attIdx) {
        for (int defIdx = 0; defIdx < histories.at(targetSol).size(); ++defIdx) {
            for (int tIdx : theories) {
                ignore = false;
#ifdef DEBUG                
                auto start = chrono::high_resolution_clock::now();
#endif
                if (tIdx==mdp.non_moralTheoryIdx) { continue;}
                // Check if target Argument has already been attacked...
                auto& potAttacks = (*attacksByTarget)[targetSol];
                for (Attack& att : potAttacks) {
                    // Looping through all attacks to make this check is naive, though only 0.18% of MEHR computation time currently.
                    if (att.targetHistoryIdx==defIdx && att.theoryIdx==tIdx) {
                        ignore = true;
                        break;
                    }
                }
#ifdef DEBUG                
                auto end = chrono::high_resolution_clock::now();
                search_time += chrono::duration_cast<chrono::microseconds>(end-start).count();
#endif

                // If target argument is already attacked, don't try and add to it.
                if (ignore) {continue;}
#ifdef DEBUG                
                start = chrono::high_resolution_clock::now();
#endif
                int result = mdp.theories[tIdx]->attack(histories.at(sourceSol).at(attIdx)->worth, histories.at(targetSol).at(defIdx)->worth);
                if (result==1) {
                    // Store this attack.
                    Attack att = Attack(sourceSol, targetSol, tIdx, attIdx, defIdx);
                    potAttacks.push_back(att);
                    // Add to non-acceptability.
                    (*non_accept)[targetSol] += histories.at(targetSol).at(defIdx)->probability;
                    //std::cout << "     Theory Id " << tIdx << ". Outcome with " << histories->at(targetSol)->at(defIdx)->worth.toString() << " attacks outcome with" << histories->at(targetSol)->at(defIdx)->worth.toString() << " for Prob +" << histories->at(targetSol)->at(defIdx)->probability << " = " << (*non_accept)[targetSol] << std::endl;
                }
#ifdef DEBUG                
                end = chrono::high_resolution_clock::now();
                attack_time += chrono::duration_cast<chrono::microseconds>(end-start).count();
#endif
                //TODO optimise to add attack if result==-1
            }
        }
    }
#ifdef DEBUG                
    auto totalCFAEnd = chrono::high_resolution_clock::now();
    total_CFA += chrono::duration_cast<chrono::microseconds>(totalCFAEnd-totalCFAStart).count();
#endif

}

