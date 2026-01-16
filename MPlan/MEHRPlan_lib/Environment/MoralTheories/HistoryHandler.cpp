//
// Created by Simon Kolker on 29/12/2025.
//
#include "HistoryHandler.hpp"
#include "QValue.hpp"
#include "ExtractHistories.hpp"

// Sorts histories for each policy with attack relation.
// Stores history indices in descending order in orderedHistories
// This is used copied for Absolutism, Utilitarianism.

void SortHistories::AddPolicyHistories(std::vector<std::vector<History*>> &histories) {
    for (const auto & policyHistories : histories) {
        std::vector<size_t> ordered(policyHistories.size(),0);
        std::iota(ordered.begin(), ordered.end(), 0);

        // Sort by history comparisons
        std::sort(ordered.begin(), ordered.end(),
            [&policyHistories, this](const int& lhs, const int& rhs) {
                // Use attack relation to check order.
                int r = rMehrTheory.attack(policyHistories[lhs]->worth, policyHistories[rhs]->worth);
                bool final = r==1;
                return final; // descending order. r=-1 means rhs beats lhs. Means rhs goes before.
        });
        orderedHistories.emplace_back(std::move(ordered));
    }
}

Attack SortHistories::CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>>& histories) {
    bool findAttack = false;
    auto& attHistories = orderedHistories[a.sourcePolicyIdx];
    auto& defHistories =orderedHistories[a.targetPolicyIdx];

    // Grab defender histories from the front to get the best defender first.
    for (size_t def_place = 0; def_place < defHistories.size(); ++def_place) {
        size_t defIdx = defHistories[def_place];
        // Grab attacker histories from the back to get worst attacker first.
        // TODO Should this not be att_place <= 0, since we do want to run once w/ 0?
        for (int att_place = attHistories.size()-1; att_place >= 0; --att_place) {
            size_t attIdx = attHistories[att_place];
            // Use the best defender history and worst attacker history.
            auto& attackerHistoryW = histories.at(a.sourcePolicyIdx).at(attIdx)->worth;
            auto& defenderHistoryW = histories.at(a.targetPolicyIdx).at(defIdx)->worth;
            int result = rMehrTheory.attack(attackerHistoryW, defenderHistoryW);
            if (result==1) {
                findAttack=true;
                for (auto i = def_place; i < defHistories.size(); ++i) {
                    a.addEdge(attIdx, defHistories[i], histories.at(a.targetPolicyIdx).at(defHistories[i])->probability);
                }
                defHistories.resize(def_place);
                break;
            }
        }
        if (findAttack) { break; }
    }
    return a;
}

void SortHistories::InitMEHR(std::vector<std::vector<History*>> &histories) {
    orderedHistories.clear();
    orderedHistories.reserve(histories.size());
    AddPolicyHistories(histories);
}
