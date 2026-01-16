//
// Created by Simon Kolker on 29/04/2025.
//
#include "Maximin.hpp"
#include "QValue.hpp"
#include "ExtractHistories.hpp"
#include "Logger.hpp"

// From this theory's Moral Considerations, finds the minimum utility for each QValue.
// Returns 1 if the minimum greater for qv1; -1 if greater for qv2; 0 if equal.
int MEHRMaximin::attack(QValue& qv1, QValue& qv2) {
    // 1. Find minimum for each qvalue, for our considerations.
    // TODO Could probably cache minimums.
    double qv1_min = std::numeric_limits<double>::max();
    double qv2_min = std::numeric_limits<double>::max();
    ExpectedUtility* qv1_curr;
    ExpectedUtility* qv2_curr;
    for (size_t c_idx : considerations) {
        qv1_curr = static_cast<ExpectedUtility*>(qv1.expectations[c_idx].get());
        qv2_curr = static_cast<ExpectedUtility*>(qv2.expectations[c_idx].get());
        if (qv1_curr->value < qv1_min) {
            qv1_min = qv1_curr->value; // TODO Could weigh these by probability? Make it easier to switch.
        }
        if (qv2_curr->value < qv2_min) {
            qv2_min = qv2_curr->value;
        }
    }
    // 2. Winner has maximum minimum.
    if (qv1_min > qv2_min) {
        return 1;
    }
    if (qv1_min < qv2_min) {
        return -1;
    }
    // If neither, draw.
    return 0;

}

int MEHRMaximin::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}

void MEHRMaximin::InitMEHR(std::vector<std::vector<History*>> &histories) {
    attacks = std::vector<std::unordered_set<size_t>>(histories.size());
}
void MEHRMaximin::AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) {
    throw std::runtime_error("MEHRMaximin::AddPoliciesForMEHR: Not implemented");
}

// Minimax compares all histories.
Attack MEHRMaximin::CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>> &histories) {
    double targetNonAccept = 0;
    for (int attIdx = 0; attIdx < histories.at(a.sourcePolicyIdx).size(); ++attIdx) {
        for (int defIdx = 0; defIdx < histories.at(a.targetPolicyIdx).size(); ++defIdx) {
            // If target argument is already attacked, don't try to add to it.
            if (attacks[a.targetPolicyIdx].contains(defIdx)) {
                continue;
            }
            int result = attack(histories.at(a.sourcePolicyIdx).at(attIdx)->worth, histories.at(a.targetPolicyIdx).at(defIdx)->worth);
            Log::writeFormatLog(Trace, "Attacker Policy {} @ Hist {} vs Defender Policy {} @ Hist {}. Result is {}", a.sourcePolicyIdx, histories.at(a.sourcePolicyIdx).at(attIdx)->worth.toString(), a.targetPolicyIdx, histories.at(a.targetPolicyIdx).at(defIdx)->worth.toString(), result);
            if (result==1) {
                // Store this attack.
                size_t t = attacks[a.targetPolicyIdx].size();
                attacks[a.targetPolicyIdx].insert(defIdx);
                if (t!=attacks[a.targetPolicyIdx].size()) {
                    a.addEdge(static_cast<size_t>(attIdx), static_cast<size_t>(defIdx), histories.at(a.targetPolicyIdx).at(defIdx)->probability);
                } else {
                    a.addEdge((size_t)attIdx, (size_t)defIdx);
                }

                a.HistoryEdges.push_back({(size_t)attIdx, (size_t)defIdx});
                // Add to non-accceptability.
                Log::writeFormatLog(Debug, Green, "***Attacker Policy {} @ Hist {} ATTACKS Defender Policy {} @ Hist {} with Pr={}", a.sourcePolicyIdx, histories.at(a.sourcePolicyIdx).at(attIdx)->worth.toString(), a.targetPolicyIdx, histories.at(a.targetPolicyIdx).at(defIdx)->worth.toString(), histories.at(a.targetPolicyIdx).at(defIdx)->probability);

            }
        }
    }

    return a;
}