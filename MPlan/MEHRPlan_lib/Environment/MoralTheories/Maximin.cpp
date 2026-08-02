//
// Created by Simon Kolker on 29/04/2025.
//
#include "Maximin.hpp"
#include "History.hpp"
#include "QValue.hpp"
#include "Logger.hpp"

// From this theory's Moral Considerations, finds the minimum utility for each QValue.
// Returns 1 if the minimum greater for qv1; -1 if greater for qv2; 0 if equal.
int MEHRMaximin::attack(QValue& qv1, QValue& qv2) {
    double qv1_min = std::numeric_limits<double>::max();
    double qv2_min = std::numeric_limits<double>::max();

    // Fast path: most comparisons may be decided by the minimum alone.
    for (const size_t c_idx : considerations) {
        const double value1 =
            static_cast<const ExpectedUtility*>(
                qv1.expectations[c_idx].get()
            )->value;

        const double value2 =
            static_cast<const ExpectedUtility*>(
                qv2.expectations[c_idx].get()
            )->value;

        qv1_min = std::min(qv1_min, value1);
        qv2_min = std::min(qv2_min, value2);
    }

    if (qv1_min > qv2_min) {
        return 1;
    }

    if (qv1_min < qv2_min) {
        return -1;
    }

    // Slow path: the minima tie, so compare the complete sorted sequences.
    std::vector<double> qv1_values;
    std::vector<double> qv2_values;

    qv1_values.reserve(considerations.size());
    qv2_values.reserve(considerations.size());

    for (const size_t c_idx : considerations) {
        qv1_values.push_back(
            static_cast<const ExpectedUtility*>(
                qv1.expectations[c_idx].get()
            )->value
        );

        qv2_values.push_back(
            static_cast<const ExpectedUtility*>(
                qv2.expectations[c_idx].get()
            )->value
        );
    }

    std::sort(qv1_values.begin(), qv1_values.end());
    std::sort(qv2_values.begin(), qv2_values.end());

    // Index 0 is already known to tie, so begin at index 1.
    for (size_t i = 1; i < qv1_values.size(); ++i) {
        if (qv1_values[i] > qv2_values[i]) {
            return 1;
        }

        if (qv1_values[i] < qv2_values[i]) {
            return -1;
        }
    }

    return 0;
}

int MEHRMaximin::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}

void MEHRMaximin::InitMEHR(policy_hists &histories) {
    attacks = std::vector<std::unordered_set<size_t>>(histories.size());
}
void MEHRMaximin::AddPoliciesForMEHR(policy_hists &histories) {
    throw std::runtime_error("MEHRMaximin::AddPoliciesForMEHR: Not implemented");
}

// Minimax compares all histories.
Attack MEHRMaximin::CriticalQuestionOne(Attack& a, policy_hists &histories) {
    double targetNonAccept = 0;
    for (int attIdx = 0; attIdx < histories.at(a.sourcePolicyIdx).size(); ++attIdx) {
        for (int defIdx = 0; defIdx < histories.at(a.targetPolicyIdx).size(); ++defIdx) {
            // If target argument is already attacked, don't try to add to it.
            if (attacks[a.targetPolicyIdx].contains(defIdx)) {
                continue;
            }
            int result = attack(histories.at(a.sourcePolicyIdx).at(attIdx)->mWorth, histories.at(a.targetPolicyIdx).at(defIdx)->mWorth);
            if (result==1) {
                // Store this attack.
                size_t t = attacks[a.targetPolicyIdx].size();
                attacks[a.targetPolicyIdx].insert(defIdx);
                if (t!=attacks[a.targetPolicyIdx].size()) {
                    a.addEdge(static_cast<size_t>(attIdx), static_cast<size_t>(defIdx), histories.at(a.targetPolicyIdx).at(defIdx)->probability);
                } else {
                    a.addEdge((size_t)attIdx, (size_t)defIdx);
                }

                a.HistoryEdges.emplace_back((size_t)attIdx, (size_t)defIdx);
                // Add to non-accceptability.

            }
        }
    }

    return a;
}