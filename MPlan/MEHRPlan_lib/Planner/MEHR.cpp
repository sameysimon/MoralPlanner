//
// Created by Simon Kolker on 30/08/2024.
//
#include "MEHR.hpp"
#include <iostream>
#include <chrono>
#include <ranges>

void MEHR::findNonAccept(NonAcceptability &non_accept) {
    non_accept = NonAcceptability(mdp.mehr_theories.size(), policies.size());
    // Iterate across theories.
    for (size_t rank = 0; rank < mdp.groupedTheoryIndices.size(); ++rank) {
        auto currTheories = mdp.groupedTheoryIndices[rank];
        for (auto &theoryIdx : currTheories) {

            // Find the maximally preferred policies by expected worth for this theory.
            vector<size_t> bestPolicies = MaxPolicyForTheory(theoryIdx, rank);
            bestExpectedPolicyByTheory[theoryIdx] = bestPolicies;

            // Mark the rank at which these policies are dominant.
            for (size_t pi_idx : bestPolicies) {
                if (invulnerablePolicies.find(pi_idx) == invulnerablePolicies.end()) {
                    invulnerablePolicies[pi_idx] = rank;
                }
            }

            // Launch attacks on less preferred theories, unless they are invulnerable
            for (size_t pi_idx=0; pi_idx < policies.size(); ++pi_idx) {
                // Skip if best policy by this rank (we are doing this check anyway after)
                if (std::find(bestPolicies.begin(), bestPolicies.end(), pi_idx) != bestPolicies.end()) { continue; }
                // Attack if target policy not invulnerable, or only invulnerable at lower/equal rank then launch attack.
                if (invulnerablePolicies.find(pi_idx) == invulnerablePolicies.end() || invulnerablePolicies[pi_idx] >= rank) {
                    auto a = mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(bestPolicies[0], pi_idx, histories);
                    non_accept.addNonAccept(theoryIdx, pi_idx, a.p);
                    if (!a.isUndefined()) { attacks[pi_idx].push_back(a); }
                }
            }
            // IF CQ2 launches attacks when expectations are equal: (Remove otherwise!!!)
            attackBetweenBestPolicies(non_accept, rank, theoryIdx, bestPolicies);
        }
    }
    doneMEHR=true;
}
// Finds the maximally preferred policy for moral theory with index theoryIdx.
vector<size_t> MEHR::MaxPolicyForTheory(unsigned long& theoryIdx, size_t rank) {
    vector<size_t> bestPolicies;
    bestPolicies.push_back(0);
    for (size_t pi_idx=1; pi_idx < policies.size(); ++pi_idx) {
        int r = PolicyCompare(theoryIdx, rank, pi_idx, bestPolicies[0]);
        if (r==1) {
            // New best policy pi_idx
            bestPolicies.clear();
            bestPolicies.push_back(pi_idx);
        } else if (r==0) {
            // Policy pi_idx equal to existing best policy; add it to the group.
            bestPolicies.push_back(pi_idx);
        }
    }
    return bestPolicies;
}

// For theory at theoryIdx, policy at index left expected to beat policy at index right for ranks less than or equal to toRank.
// Settles draw with most preferred history.
int MEHR::PolicyCompare(const ushort theoryIdx, const ushort rankIdx, const size_t& left, const size_t& right) {
    // If defeated at a more important rank, then cannot be a best policy for this theory.
    if (rankIdx > 0 && CheckAttackToRank(rankIdx-1, left, right)==-1) {
        return -1;
    }
    int r = mdp.mehr_theories[theoryIdx]->CriticalQuestionTwo(policies[left]->worth[0], policies[right]->worth[0]);
    if (r==1) {
        return 1;
    }
    if (r==-1) {
        return -1;
    }

    // Settle draws by picking policy with the greatest worth single history.
    auto bestLeftHistory = max_element(histories[left].begin(), histories[left].end(), [this, theoryIdx](History *local_left, History *local_right) {
        return local_left->worth.expectations[theoryIdx]->compare(*local_right->worth.expectations[theoryIdx]) == 1;
    });
    auto bestRightHistory = max_element(histories[right].begin(), histories[right].end(), [this, theoryIdx](History *local_left, History *local_right) {
        return local_left->worth.expectations[theoryIdx]->compare(*local_right->worth.expectations[theoryIdx]) == 1;
    });

    return (*bestLeftHistory.base())->worth.expectations[theoryIdx]->compare(*(*bestRightHistory.base())->worth.expectations[theoryIdx]);
}
// Checks if policy at index left expected to beat policy at index right for ranks less than or equal to toRank.
int MEHR::CheckAttackToRank(const ushort toRank, const size_t& left, const size_t& right) {
    int oldRes=0;
    for (size_t rIdx = 0; rIdx < toRank; ++rIdx) {
        auto currTheories = mdp.groupedTheoryIndices[rIdx];
        for (auto &theoryIdx : currTheories) {
            int res = mdp.mehr_theories[theoryIdx]->CriticalQuestionTwo(policies[left]->worth[0], policies[right]->worth[0]);
            if (res != 0 && oldRes != 0 && res != oldRes) {
                return 0; // Draw
            }
            if (res != 0) {
                oldRes = res;
            }
        }
        if (oldRes != 0) {
            return oldRes;
        }
    }
    return 0; // No attacks were found, so must be a draw.
}

void MEHR::attackBetweenBestPolicies(NonAcceptability& non_accept, size_t rank, unsigned long& theoryIdx, vector<size_t>& bestPolicies) {
    for (size_t pi_one_idx : bestPolicies) {
        for (size_t pi_two_idx : bestPolicies) {
            if (pi_one_idx == pi_two_idx) { continue; }
            // Do not attack if target's expectation is invulnerable at a greater rank.
            if (invulnerablePolicies[pi_two_idx] < rank) { continue; }
            auto a = mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(pi_one_idx, pi_two_idx, histories);
            if (!a.isUndefined()) { attacks[pi_two_idx].push_back(a); }
            non_accept.addNonAccept(theoryIdx, pi_one_idx, a.p);
        }
    }
}


/*
 * Below used in Explainability bit.
 */

void MEHR::addPoliciesToMEHR(NonAcceptability &non_accept, vector<Policy*> &newPolicies, vector<vector<History*>> &newHistories) {
    if (!doneMEHR) { return; }
    for (auto t : mdp.mehr_theories) {
        t->AddPoliciesForMEHR(newHistories);
    }
    for (size_t newPiIdx=0; newPiIdx < newPolicies.size(); ++newPiIdx) {
        addPolicyToMEHR(newPolicies[newPiIdx], newHistories[newPiIdx], non_accept);
    }
}

void MEHR::addPolicyToMEHR(Policy *pi, vector<History*> &h, NonAcceptability &non_accept) {
    size_t newPolicyIdx = policies.size();
    // TODO this is updating the runner's policy/histories which seems wrong. The runner should update them. But also, MEHR needs to know which policies are new. Hmm.
    policies.push_back(pi);
    histories.push_back(h);
    attacks.emplace_back();

    if (histories.size() != policies.size()) {
        throw runtime_error("MEHR::addPolicyToMEHR: Histories and policies size desync.");
    }
    non_accept.appendPolicy();

    // Iterate across moral theories in decreasing preference order.
    for (size_t rank = 0; rank < mdp.groupedTheoryIndices.size(); ++rank) {
        auto currTheories = mdp.groupedTheoryIndices[rank];
        for (auto &theoryIdx : currTheories) {
            // Compare with old best policy
            size_t oldBestExpPi = bestExpectedPolicyByTheory[theoryIdx][0];
            int comp = PolicyCompare(theoryIdx, rank, newPolicyIdx, oldBestExpPi);

            if (comp == 1) {
                // Remove mark that said old policies are invulnerable. Add mark to new policy
                for (size_t x : bestExpectedPolicyByTheory[theoryIdx]) {
                    invulnerablePolicies.erase(x);
                }
                invulnerablePolicies[newPolicyIdx] = rank;

                // Add the new, dominant policy.
                bestExpectedPolicyByTheory[theoryIdx].clear();
                bestExpectedPolicyByTheory[theoryIdx].push_back(newPolicyIdx);
            } else if (comp == 0) {
                // Same policy as old policy
                invulnerablePolicies[newPolicyIdx] = rank;
                bestExpectedPolicyByTheory[theoryIdx].push_back(newPolicyIdx);
                BestPoliciesAttackPi(non_accept, rank, theoryIdx, bestExpectedPolicyByTheory[theoryIdx], newPolicyIdx);
                PiAttackBestPolicies(non_accept, rank, theoryIdx, bestExpectedPolicyByTheory[theoryIdx], newPolicyIdx);
            } else if (comp == -1) {
                // New policy is worse.
                BestPoliciesAttackPi(non_accept, rank, theoryIdx, bestExpectedPolicyByTheory[theoryIdx], newPolicyIdx);
            }

        }
    }
}

// Check for attacks from the set of bestPolicies on the policy with index pi_idx
void MEHR::BestPoliciesAttackPi(NonAcceptability& non_accept, size_t rank, unsigned long& theoryIdx, vector<size_t>& bestPolicies, size_t pi_idx) {
    for (size_t bestIdx : bestPolicies) {
        if (pi_idx == bestIdx) { continue; }
        if (invulnerablePolicies[pi_idx] < rank) { continue; }
        auto a = mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(bestIdx, pi_idx, histories);
        non_accept.addNonAccept(theoryIdx, pi_idx, a.p);
        if (!a.isUndefined()) { attacks[pi_idx].push_back(a); }
    }
}

// Check for attacks from the policy with index pi_idx on the set of bestPolicies
void MEHR::PiAttackBestPolicies(NonAcceptability& non_accept, size_t rank, unsigned long& theoryIdx, vector<size_t>& bestPolicies, size_t pi_idx) {
    // Check for attacks on the new policy
    for (size_t bestIdx : bestPolicies) {
        if (pi_idx == bestIdx) { continue; }
        if (invulnerablePolicies[pi_idx] < rank) { continue; }
        auto a = mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(pi_idx, bestIdx, histories);
        non_accept.addNonAccept(theoryIdx, bestIdx, a.p);
        if (!a.isUndefined()) { attacks[bestIdx].push_back(a); }
    }
}


//
// MAIN START POINT
//

/*
/*
void MEHR::oldFindNonAccept(vector<double> &non_accept) {
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

