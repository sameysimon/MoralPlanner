//
// Created by Simon Kolker on 30/08/2024.
//
#include "MEHR.hpp"
#include <iostream>
#include <chrono>
#include <ranges>


MEHR::MEHR(MDP& mdp, vector<unique_ptr<Policy>> &policies_, vector<vector<History*>> &histories_) : mdp(mdp), policies(policies_), histories(histories_) {
    auto t1 = std::chrono::high_resolution_clock::now();
    attacks.resize(policies.size());
    // Prepare each moral theory for MEHR based on these theories.
    for (auto t : mdp.mehr_theories) {
        t->InitMEHR(histories);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    init_time += std::chrono::duration_cast<time_metric>(t2 - t1).count();
}


void MEHR::RankPoliciesByTheory(size_t theoryIdx) {
    // sortedIndices[rank] = pi_idx
    vector<size_t>& sortedIndices = policyOrderByTheory[theoryIdx];
    sortedIndices.resize(policies.size());
    iota(sortedIndices.begin(), sortedIndices.end(), 0);
    sort(sortedIndices.begin(), sortedIndices.end(),
        [&](const size_t& lhs, const size_t& rhs) {
            auto& lhsWorth = policies[lhs]->worth[0];
            auto& rhsWorth = policies[rhs]->worth[0];
            return 1 == CQ2CompareWithRank(lhsWorth, rhsWorth, theoryIdx);
        });
    // policyRanks[pi_Idx] = rank/pos in order
    vector<size_t>& policyRanks = policyRanksByTheory[theoryIdx];
    policyRanks.resize(policies.size());
    size_t rank = 0;
    policyRanks[sortedIndices[0]] = rank;
    for (size_t pi_idx = 0; pi_idx < sortedIndices.size(); pi_idx++) {
        policyRanks[pi_idx] = pi_idx;
    }

}

void MEHR::BestPolicyOutcomeByTheory(size_t theoryIdx) {
    // bestOutcomeIdxs[pi_idx] = index of history with best worth
    vector<size_t>& bestOutcomeIdxs = posBestOutcomeByTheory[theoryIdx];
    // bestOutcomeIdxs[pi_idx] = index of policy with the best history worth for this policy, equal and better.
    vector<size_t>& bestAccOutcomeIdxs = accPosBestOutcomeByTheory[theoryIdx];
    size_t maxHistory_piIDx = 0;
    size_t maxHistory_hIDx = 0;
    for (auto pi_idx : policyOrderByTheory[theoryIdx]) {
        auto currBestIdx = getBestHistoryForPolicy(theoryIdx, pi_idx);
        bestOutcomeIdxs[pi_idx] = currBestIdx;
        if (mdp.mehr_theories[theoryIdx]->attack(histories[pi_idx][currBestIdx]->worth, histories[maxHistory_piIDx][maxHistory_hIDx]->worth)) {
            maxHistory_piIDx = pi_idx;
            maxHistory_hIDx = currBestIdx;
        }
        bestAccOutcomeIdxs[theoryIdx] = maxHistory_piIDx;
    }

}

// Weak total order over outcome/action preferences.
// Thus, for attacks from one action to another, need only best outcome from attacker.

void MEHR::findNonAccept(NonAcceptability &non_accept) {
    non_accept = NonAcceptability(mdp.mehr_theories.size(), policies.size());
    // Iterate over moral theories in lexi-descending.
    // Sort policies by expected worth in ascending worth; store in policyOrderByTheory[theoryIdx].
    // Descend policyOrderByTheory[theoryIdx]; collect policy with the best history so far as accPosBestOutcomeByTheory[theoryIdx].
    // Compare
    for (auto & currTheories : mdp.groupedTheoryIndices) {
        for (auto &theoryIdx : currTheories) {
            RankPoliciesByTheory(theoryIdx);
            BestPolicyOutcomeByTheory(theoryIdx);
            for (size_t pi_idx : policyOrderByTheory[theoryIdx]) {
                // Find the worst policy that is better than current.
                size_t worstBetter_piIdx = policyRanksByTheory[theoryIdx][pi_idx];
                QValue& currAttackWorth = *policies[policyOrderByTheory[theoryIdx][pi_idx]]->getExpectationPtr();
                bool found = false;
                do {
                    worstBetter_piIdx--;
                    auto& firstAttWorth = *policies[policyOrderByTheory[theoryIdx][worstBetter_piIdx]]->getExpectationPtr();
                    if (1 == mdp.mehr_theories[theoryIdx]->CriticalQuestionTwo(firstAttWorth, currAttackWorth)) {
                        found = true;
                    }
                }
                while (worstBetter_piIdx != 0 and !found);
                // If no better policy expectation, then no valid attack on this policy.
                if (!found) {
                    continue;
                }
                // Get policy with the best history equal/preferred to the worst better policy and attack with it.
                size_t bestAttacker_piIdx = accPosBestOutcomeByTheory[theoryIdx][worstBetter_piIdx];
                Attack att(bestAttacker_piIdx, pi_idx, theoryIdx);
                mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(att, histories);
                if (!att.isUndefined()) { attacks[pi_idx].push_back(att); }
                non_accept.addNonAccept(theoryIdx, pi_idx, att.p);

            }
        }
    }
    doneMEHR=true;
}

void MEHR::SortExpFindNonAccept(NonAcceptability &non_accept) {
    for (size_t theoryIdx=0; theoryIdx < mdp.mehr_theories.size(); theoryIdx++) {
        for (size_t pi_idx1 = 0; pi_idx1 < policies.size(); pi_idx1++) {
            auto& pi1QV = *policies[pi_idx1]->getExpectationPtr();
            bool found_attacker = false;
            size_t best_pi_Idx = 0;
            size_t best_h_Idx = 0;
            for (size_t pi_idx2 = 0; pi_idx2 < policies.size(); pi_idx2++) {
                if (pi_idx1==pi_idx2) { continue; }
                auto &currQV = *policies[pi_idx2]->getExpectationPtr();
                auto r = mdp.mehr_theories[theoryIdx]->CriticalQuestionTwo(currQV, pi1QV);
                if (r!=1) { continue; }
                auto currBestIdx = getBestHistoryForPolicy(theoryIdx, pi_idx2);
                if (1==mdp.mehr_theories[theoryIdx]->attack(histories[pi_idx2][currBestIdx]->worth, histories[best_pi_Idx][best_h_Idx]->worth)) {
                    best_pi_Idx = pi_idx2;
                    best_h_Idx = currBestIdx;
                    found_attacker = true;
                }
            }
            if (found_attacker) {
                CQ1AndAddAttack(best_pi_Idx, pi_idx1, theoryIdx, non_accept);
            }
        }
    }
}


void MEHR::SlowFindNonAccept(NonAcceptability &non_accept) {
    for (size_t theoryIdx=0; theoryIdx < mdp.mehr_theories.size(); theoryIdx++) {
        for (size_t pi_idx1 = 0; pi_idx1 < policies.size(); pi_idx1++) {
            QValue* qv1 = policies[pi_idx1]->getExpectationPtr();
            QValue* qv2;
            for (size_t pi_idx2 = pi_idx1+1; pi_idx2 < policies.size(); pi_idx2++) {
                qv2 = policies[pi_idx2]->getExpectationPtr();
                auto cq2 = CQ2CompareWithRank(*qv1, *qv2, theoryIdx);
                if (cq2==1) {
                    CQ1AndAddAttack(pi_idx1, pi_idx2, theoryIdx, non_accept);
                } else if (cq2==-1) {
                    CQ1AndAddAttack(pi_idx2, pi_idx1, theoryIdx, non_accept);
                }
            }
        }
    }
    doneMEHR=true;
}
// If cq1 attacks cq2 at this theory by CQ2, and no preferred theory attacks in the other direction, then cq1 attacks cq2 (return 1).
// Reverse returns -1; neither returns 0.
int MEHR::CQ2CompareWithRank(QValue& qv1, QValue& qv2, size_t theoryIdx) {
    //auto t1 = std::chrono::high_resolution_clock::now();
    // Get the inputted theory's valuation.
    auto r = mdp.mehr_theories[theoryIdx]->CriticalQuestionTwo(qv1, qv2);
    // Compare from the best theory to the current rank.
    for (size_t rankIdx = 0; rankIdx < mdp.groupedTheoryIndices.size(); rankIdx++) {
        for (auto &currTheoryIdx : mdp.groupedTheoryIndices[rankIdx]) {
            if (mdp.mehr_theories[currTheoryIdx]->mRank >= mdp.mehr_theories[theoryIdx]->mRank) {
                break;// If found inputted theory's rank, theory defences are ignored.
            }
            auto def = mdp.mehr_theories[currTheoryIdx]->CriticalQuestionTwo(qv1, qv2);
            if (r != def && r!= 0) {
                return 0;
            }
        }
    }
    //auto t2 = std::chrono::high_resolution_clock::now();
    //cq2_time += std::chrono::duration_cast<time_metric>(t2 - t1).count();
    return r;
}
void MEHR::CQ1AndAddAttack(size_t source_pi, size_t target_pi, size_t theoryIdx, NonAcceptability &non_accept) {
    //auto t1 = std::chrono::high_resolution_clock::now();
    Attack att = Attack(source_pi, target_pi, theoryIdx);
    mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(att, histories);
    if (att.p != 0) { attacks[target_pi].push_back(std::move(att)); }
    non_accept.addNonAccept(theoryIdx, target_pi, att.p);
    //auto t2 = std::chrono::high_resolution_clock::now();
    //cq1_time += std::chrono::duration_cast<time_metric>(t2 - t1).count();
}


size_t MEHR::getBestHistoryForPolicy(size_t theoryIdx, size_t policyIdx) {
    size_t bestIdx = 0;
    for (size_t i = 1; i < histories[policyIdx].size(); ++i) {
        auto a = mdp.mehr_theories[theoryIdx]->attack(histories[policyIdx][i]->worth, histories[policyIdx][bestIdx]->worth);
        if (a==1) {
            bestIdx = i;
        }
    }
    return bestIdx;
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
            Attack a(pi_one_idx, pi_two_idx, theoryIdx);
            mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(a, histories);
            if (!a.isUndefined()) { attacks[pi_two_idx].push_back(a); }
            non_accept.addNonAccept(theoryIdx, pi_one_idx, a.p);
        }
    }
}


/*
 * Below used in Explainability bit.
 */

void MEHR::addPoliciesToMEHR(NonAcceptability &non_accept, vector<unique_ptr<Policy>> &newPolicies, vector<vector<History*>> &newHistories) {
    if (!doneMEHR) { return; }
    for (auto t : mdp.mehr_theories) {
        t->AddPoliciesForMEHR(newHistories);
    }
    for (size_t newPiIdx=0; newPiIdx < newPolicies.size(); ++newPiIdx) {
        addPolicyToMEHR(newPolicies[newPiIdx], newHistories[newPiIdx], non_accept);
    }
}

void MEHR::addPolicyToMEHR(unique_ptr<Policy>& pi, vector<History*> &h, NonAcceptability &non_accept) {
    size_t newPolicyIdx = policies.size();
    // TODO this is updating the runner's policy/histories which seems wrong. The runner should update them. But also, MEHR needs to know which policies are new. Hmm.
    policies.push_back(std::move(pi));
    histories.push_back(h);
    attacks.emplace_back();
    non_accept.appendPolicy();
    auto& newQv = policies[newPolicyIdx]->worth[0];
    if (histories.size() != policies.size()) {
        throw runtime_error("MEHR::addPolicyToMEHR: Histories and policies size desync.");
    }
    for (size_t theoryIdx=0; theoryIdx < mdp.mehr_theories.size(); theoryIdx++) {
        for (size_t pi_idx = 0; pi_idx < policies.size(); pi_idx++) {
            auto& qv = policies[pi_idx]->worth[0];
            auto cq2 = CQ2CompareWithRank(newQv, qv, theoryIdx);
            if (cq2==1) {
                CQ1AndAddAttack(newPolicyIdx, pi_idx, theoryIdx, non_accept);
            } else if (cq2==-1) {
                CQ1AndAddAttack(pi_idx, newPolicyIdx, theoryIdx, non_accept);
            }
        }
    }
}

// Check for attacks from the set of bestPolicies on the policy with index pi_idx
void MEHR::BestPoliciesAttackPi(NonAcceptability& non_accept, size_t rank, unsigned long& theoryIdx, vector<size_t>& bestPolicies, size_t pi_idx) {
    for (size_t bestIdx : bestPolicies) {
        if (pi_idx == bestIdx) { continue; }
        if (invulnerablePolicies[pi_idx] < rank) { continue; }
        Attack a(bestIdx, pi_idx, theoryIdx);
        mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(a, histories);
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
        Attack a(bestIdx, pi_idx, theoryIdx);
        mdp.mehr_theories[theoryIdx]->CriticalQuestionOne(a, histories);
        non_accept.addNonAccept(theoryIdx, bestIdx, a.p);
        if (!a.isUndefined()) { attacks[bestIdx].push_back(a); }
    }
}


