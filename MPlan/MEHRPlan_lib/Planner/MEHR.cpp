//
// Created by Simon Kolker on 30/08/2024.
//
#include "MEHR.hpp"
#include <iostream>
#include <chrono>
#include <ranges>


// Builds hRT (histories' Ranked by Theories)
// For each theory, each policy, all histories in descending order. Most preferable to least preferable.
// Sort costs HlogH (for H histories). Thus, T*PI*H*logH.
// hRT[theoryIdx][piIdx] = [histIdx1, histIdx2, ...]
void MEHR::SortHistories(vector<vector<History*>>& histories, vector<vector<vector<int>>>& hRT) const {
    size_t numOfPolicies = histories.size();
    hRT.clear();
    hRT.reserve(mdp.considerations.size());

    for (int tIdx = 0; tIdx < mdp.mehr_theories.size(); tIdx++) {
        // 2D vector. For each policy, ordered list of history indices.
        vector policySortedHistories(numOfPolicies, vector<int>());
        hRT.push_back(policySortedHistories);

        for (int pIdx = 0; pIdx < numOfPolicies; pIdx++) {
            // Set to number of histories
            hRT[tIdx][pIdx].resize(histories[pIdx].size());
            // Default order is 0,1,2,... number of histories for this policy
            iota(hRT[tIdx][pIdx].begin(), hRT[tIdx][pIdx].end(), 0);

            // Sort by history comparisons
            sort(hRT[tIdx][pIdx].begin(), hRT[tIdx][pIdx].end(),
                [histories, pIdx, tIdx, this](const int& lhs, const int& rhs) {
                    // Use attack relation to check order.
                    int r = mdp.mehr_theories[tIdx]->attack(histories[pIdx][lhs]->worth, histories[pIdx][rhs]->worth);
                    return r==1;// descending order. r=-1 means rhs beats lhs. Means rhs goes before.
            });
        }
    }
}

//
// MAIN START POINT
//
void MEHR::findNonAccept(vector<double> &non_accept) {
    const ushort totalPolicies = policyWorths.size();
    // Initialise Non-acceptability vec.
    non_accept.resize(totalPolicies);
    std::fill(non_accept.begin(), non_accept.end(), 0);

    vector<int> attackingTheories = vector<int>(); // Init theories where pi_one attacks pi_two
    vector<int> reverseTheories = vector<int>(); // Init theories where pi_two attacks pi_one
    int r = 0;

    // Compares all policies' expectations. If any 2 policies unequal, checks for attack.
    // TODO: Find all (pi, pi', t_idx) where pi CQ2 beats pi' (AND no pi'' beats pi) under t_idx. Then only do those attacks.
    for (int oneIdx=0; oneIdx<totalPolicies; ++oneIdx) {
        // The expected worth of pi_one
        QValue& oneExp = policyWorths[oneIdx];
        for (int twoIdx=oneIdx+1; twoIdx<policyWorths.size(); ++twoIdx) {
            QValue& twoExp = policyWorths[twoIdx];
            // Values that will be used.
            attackingTheories.clear();
            reverseTheories.clear();
            r = mdp.compareExpectations(oneExp, twoExp, attackingTheories, reverseTheories);

            // No greater expectations -> no attacks can be formed.
            if (r == 0) {
                continue;
            }
            // Check for attacks from policy one to policy two by attackingTheories
            if (r==1 or r==2) {
                non_accept[twoIdx] += checkForAttack(oneIdx, twoIdx, attackingTheories);
            }
            // Check for attacks from policy two to policy one by reverseTheories
            if (r==-1 or r==2) {
                non_accept[oneIdx] += checkForAttack(twoIdx, oneIdx, reverseTheories);
            }

        }
    }
}

// Generates attacks from source pi to target pi, using theories. Updates non_accept accordingly.
// Compares all sourceSol histories against all targetSol histories, for each theory.
// Once target history attacked by argument, it is skipped on future.
// Check using hash set if this.useAttackHash=true
double MEHR::checkForAttack(int sourceSol, int targetSol, vector<int>& mehrTheories) {
    double targetNonAccept=0;
    for (int tIdx : mehrTheories) {
        targetNonAccept += mdp.mehr_theories[tIdx]->CriticalQuestionOne(sourceSol, targetSol, histories);
    }
    return targetNonAccept;
}




/*
// Generates attacks from source pi to target pi, using theories. Updates non_accept accordingly.
// Compares worst history from source to best history from target.
// Increments source history to its best, then resets and decrements best.
// If attack, makes attack from all source's better histories to all target's worse histories.
// Once target history attacked by argument, it is skipped on future.
// Checks using hash set if this->useAttackHash=true
double MEHR::checkForAttack(int sourceSol, int targetSol, vector<int>& theories, vector<vector<vector<int>>> &hRT) {
    Log::writeLog(std::format("Source Pi {} attacks Target Pi {}", sourceSol, targetSol), LogLevel::Trace);
    double targetNacc=0;
    for (auto tIdx : theories) {
        bool findAttack = false;
        auto& attHistories = hRT[tIdx][sourceSol];
        auto& defHistories = hRT[tIdx][targetSol];

        for (size_t def_place = 0; def_place < defHistories.size(); ++def_place) {
            int defIdx = defHistories[def_place];
            for (auto attIt = attHistories.rbegin(); attIt != attHistories.rend(); ++attIt) {
                // Use the best defender history and worst attacker history.
                auto attackerHistoryW = histories.at(sourceSol).at(*attIt)->worth;
                auto defenderHistoryW = histories.at(targetSol).at(defIdx)->worth;
                int result = mdp.mehr_theories[tIdx]->attack(attackerHistoryW, defenderHistoryW);
                Log::writeLog(std::format("Attacker Worth {} vs Defender Worth {}. Result is {}", attackerHistoryW.toString(), defenderHistoryW.toString(), result), LogLevel::Trace);
                // There is an attack!
                if (result==1) {
                    findAttack=true;
                    for (auto i = def_place; i < defHistories.size(); ++i) {
                        targetNacc += histories.at(targetSol).at(defHistories[i])->probability;
                        Log::writeLog(std::format("   History {} beats {} w/ probability {}, theory {}", *attIt, defHistories[i], histories.at(targetSol).at(i)->probability, tIdx), LogLevel::Trace);

                    }
                    if (mdp.mehr_theories[tIdx]->orderHistories) {
                        // We can prune arguments from future considerations if histories for this theory are orderd.
                        defHistories.resize(def_place);
                    }
                    break;
                }
            }
            if (findAttack) { break; }
        }
    }
    return targetNacc;
}
*/

