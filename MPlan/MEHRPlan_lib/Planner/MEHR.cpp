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
void MEHR::sortHistories(vector<vector<History*>>& histories, vector<vector<vector<int>>>& hRT) const {
    size_t numOfPolicies = histories.size();
    hRT.clear();
    hRT.reserve(mdp.theories.size());
    for (int tIdx = 0; tIdx < mdp.theories.size(); tIdx++) {
        vector policySortedHistories(numOfPolicies, vector<int>());
        hRT.push_back(policySortedHistories);

        for (int pIdx = 0; pIdx < numOfPolicies; pIdx++) {
            // Set to number of histories
            hRT[tIdx][pIdx].resize(histories[pIdx].size());
            // Fill with indices from 0
            iota(hRT[tIdx][pIdx].begin(), hRT[tIdx][pIdx].end(), 0);
            // Sort by history comparisons
            sort(hRT[tIdx][pIdx].begin(), hRT[tIdx][pIdx].end(),
                [histories, pIdx, tIdx](const int& lhs, const int& rhs) {
                    int r = histories[pIdx][lhs]->worth.expectations[tIdx]->compare(*histories[pIdx][rhs]->worth.expectations[tIdx]);
                    return r==+1;// descending order. r=-1 means rhs beats lhs. Means rhs goes before.
            });
        }
    }
}

//
// MAIN START POINT
//
void MEHR::findNonAccept(vector<double> &non_accept) {
    // Initialise Non-acceptability vec.
    const ushort totalPolicies = policyWorths.size();
    non_accept.resize(totalPolicies);
    std::fill(non_accept.begin(), non_accept.end(), 0);

    // Sort histories for each policy, for each theory.
    auto hRT = vector<vector<vector<int>>>();
    sortHistories(histories, hRT);

    // Init theories where pi_one attacks pi_two
    vector<int> attackingTheories = vector<int>();
    // Init theories where pi_two attacks pi_one
    vector<int> reverseTheories = vector<int>();
    // Stores result of each attack
    int r = 0;

    // Compares all policies' expectations. If any 2 policies unequal, checks for attack.
    for (int oneIdx=0; oneIdx<totalPolicies; ++oneIdx) {
        // The expected worth of pi_one
        QValue& oneExp = policyWorths[oneIdx];
        for (int twoIdx=oneIdx+1; twoIdx<policyWorths.size(); ++twoIdx) {
            // Values that will be used.
            attackingTheories.clear();
            reverseTheories.clear();
            QValue& twoExp = policyWorths[twoIdx];
            // Compare expectations of policy Idx 1 to policy Idx 2
            // Store which way different theories may attack:
            r = mdp.compareExpectations(oneExp, twoExp, attackingTheories, reverseTheories);
            // No greater expectations -> no attacks can be formed.
            if (r == 0) { continue; }
            // Check for attacks from policy one to policy two by attackingTheories
            if (r==1 or r==2) {
                non_accept[twoIdx] += checkForAttack(oneIdx, twoIdx, attackingTheories, hRT);
            }
            // Check for attacks from policy two to policy one by reverseTheories
            if (r==-1 or r==2) {
                non_accept[oneIdx] += checkForAttack(twoIdx, oneIdx, reverseTheories, hRT);
            }

        }
    }
}

// Generates attacks from source pi to target pi, using theories. Updates non_accept accordingly.
// Compares all sourceSol histories against all targetSol histories, for each theory.
// Once target history attacked by argument, it is skipped on future.
// Check using hash set if this.useAttackHash=true
double MEHR::checkForAttack(int sourceSol, int targetSol, vector<int>& theories) {
    double targetNacc=0;
    for (int attIdx = 0; attIdx < histories.at(sourceSol).size(); ++attIdx) {
        for (int defIdx = 0; defIdx < histories.at(targetSol).size(); ++defIdx) {
            for (int tIdx : theories) {
                std::cout << histories[sourceSol][attIdx]->worth.toString() << std::endl;
                if (tIdx==mdp.non_moralTheoryIdx) { continue;}
                if (checkAttackExists(targetSol, defIdx, tIdx)) {
                    // If target argument is already attacked, don't try to add to it.
                    continue;
                }
                int result = mdp.theories[tIdx]->attack(histories.at(sourceSol).at(attIdx)->worth, histories.at(targetSol).at(defIdx)->worth);
                if (result==1) {
                    // Store this attack.
                    addAttack(sourceSol, targetSol, tIdx, attIdx, defIdx);
                    // Add to non-acceptability.
                    targetNacc += histories.at(targetSol).at(defIdx)->probability;
                }
            }
        }
    }
    return targetNacc;
}

// Generates attacks from source pi to target pi, using theories. Updates non_accept accordingly.
// Compares worst history from source to best from target.
// Increments source history to its best, then resets and decrements best.
// If attack, makes attack from all source's better histories to all target's worse histories.
// Once target history attacked by argument, it is skipped on future.
// Check using hash set if this.useAttackHash=true
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
                int result = mdp.theories[tIdx]->attack(histories.at(sourceSol).at(*attIt)->worth, histories.at(targetSol).at(defIdx)->worth);
                if (result==1) {
                    findAttack=true;
                    for (auto i = def_place; i < defHistories.size(); ++i) {
                        targetNacc += histories.at(targetSol).at(defHistories[i])->probability;
                        Log::writeLog(std::format("   History {} beats {} w/ probability {}, theory {}", *attIt, defHistories[i], histories.at(targetSol).at(i)->probability, tIdx), LogLevel::Trace);
                    }
                    defHistories.resize(def_place);
                    break;
                }
            }
            if (findAttack) { break; }
        }
    }
    return targetNacc;
}

