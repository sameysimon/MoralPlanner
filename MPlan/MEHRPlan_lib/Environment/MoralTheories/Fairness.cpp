//
// Created by Simon Kolker on 29/04/2026.
//

#include "Fairness.hpp"
#include "History.hpp"
#include "QValue.hpp"
#include "Logger.hpp"

// From this theory's Moral Considerations, finds the minimum utility for each QValue.
// Returns 1 if the minimum greater for qv1; -1 if greater for qv2; 0 if equal.
int MEHRFairness::attack(QValue& qv1, QValue& qv2) {
    // 1. Find minimum for each qvalue, for our considerations.
    double qv1_sum = 0.0f;
    double qv1_sq = 0.0f;
    double qv2_sum = 0.0f;
    double qv2_sq = 0.0f;
    double val;
    for (size_t c_idx : considerations) {
        val = static_cast<ExpectedUtility*>(qv1.expectations[c_idx].get())->value;
        qv1_sum += val;
        qv1_sq += val * val;
        val = static_cast<ExpectedUtility*>(qv2.expectations[c_idx].get())->value;
        qv2_sum += val;
        qv2_sq += val * val;

    }
    double result = (qv1_sq - (qv1_sum * qv1_sum) / static_cast<double>(considerations.size())) - (qv2_sq - (qv2_sum * qv2_sum) / static_cast<double>(considerations.size()));
    // 2. Winner has maximum minimum.
    if (result > 0.0) {
        return -1;
    }
    if (result < 0.0) {
        return 1;
    }
    // If neither, draw.
    return 0;

}

int MEHRFairness::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}

void MEHRFairness::InitMEHR(policy_hists &histories) {
    attacks = std::vector<std::unordered_set<size_t>>(histories.size());
}
void MEHRFairness::AddPoliciesForMEHR(policy_hists &histories) {
    throw std::runtime_error("MEHRFairness::AddPoliciesForMEHR: Not implemented");
}

// Minimax compares all histories.
Attack MEHRFairness::CriticalQuestionOne(Attack& a, policy_hists &histories) {
    double targetNonAccept = 0;
    for (int attIdx = 0; attIdx < histories.at(a.sourcePolicyIdx).size(); ++attIdx) {
        for (int defIdx = 0; defIdx < histories.at(a.targetPolicyIdx).size(); ++defIdx) {
            // If target argument is already attacked, don't try to add to it.
            if (attacks[a.targetPolicyIdx].contains(defIdx)) {
                continue;
            }
            int result = attack(histories.at(a.sourcePolicyIdx).at(attIdx)->mWorth, histories.at(a.targetPolicyIdx).at(defIdx)->mWorth);
            Log::writeFormatLog(Trace, "Attacker Policy {} @ Hist {} vs Defender Policy {} @ Hist {}. Result is {}", a.sourcePolicyIdx, histories.at(a.sourcePolicyIdx).at(attIdx)->mWorth.toString(), a.targetPolicyIdx, histories.at(a.targetPolicyIdx).at(defIdx)->mWorth.toString(), result);
            if (result==1) {
                // Store this attack.
                size_t t = attacks[a.targetPolicyIdx].size();
                attacks[a.targetPolicyIdx].insert(defIdx);
                if (t!=attacks[a.targetPolicyIdx].size()) {
                    a.addEdge(static_cast<size_t>(attIdx), static_cast<size_t>(defIdx), histories.at(a.targetPolicyIdx).at(defIdx)->probability);
                } else {
                    a.addEdge((size_t)attIdx, (size_t)defIdx);
                }
                Log::writeFormatLog(Debug, Green, "***Attacker Policy {} @ Hist {} ATTACKS Defender Policy {} @ Hist {} with Pr={}", a.sourcePolicyIdx, histories.at(a.sourcePolicyIdx).at(attIdx)->mWorth.toString(), a.targetPolicyIdx, histories.at(a.targetPolicyIdx).at(defIdx)->mWorth.toString(), histories.at(a.targetPolicyIdx).at(defIdx)->probability);

            }
        }
    }

    return a;
}