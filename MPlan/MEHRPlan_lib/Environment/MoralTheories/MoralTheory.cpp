//
// Created by Simon Kolker on 11/05/2025.
//
#include "QValue.hpp"
#include "ExtractHistories.hpp"
#include "Logger.hpp"

// Sorts histories for each policy with attack relation.
// Stores history indices in descending order in orderedHistories
// This is used copied for Absolutism, Utilitarianism.
void SortHistories::InitMEHR(std::vector<std::vector<History*>> &histories) {
    orderedHistories.clear();
    orderedHistories.reserve(histories.size());
    AddPolicyHistories(histories);
}

void SortHistories::AddPolicyHistories(std::vector<std::vector<History*>> &histories) {
    for (const auto & policyHistories : histories) {
        std::vector<size_t> ordered(policyHistories.size(),0);
        std::iota(ordered.begin(), ordered.end(), 0);
        Log::writeLog(std::format("\n"));

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

Attack SortHistories::CriticalQuestionOne(size_t sourceSol, size_t targetSol, std::vector<std::vector<History*>>& histories, size_t theoryIdx) {
    Attack a = Attack(sourceSol, targetSol, theoryIdx);
    double targetNacc = 0;
    bool findAttack = false;
    auto& attHistories = orderedHistories[sourceSol];
    auto& defHistories =orderedHistories[targetSol];

    // Grab defender histories from the front to get the best defender first.
    for (size_t def_place = 0; def_place < defHistories.size(); ++def_place) {
        size_t defIdx = defHistories[def_place];
        // Grab attacker histories from the back to get worst attacker first.
        // TODO Should this not be att_place <= 0, since we do want to run once w/ 0?
        for (int att_place = attHistories.size()-1; att_place >= 0; --att_place) {
            size_t attIdx = attHistories[att_place];
            // Use the best defender history and worst attacker history.
            auto attackerHistoryW = histories.at(sourceSol).at(attIdx)->worth;
            auto defenderHistoryW = histories.at(targetSol).at(defIdx)->worth;
            int result = rMehrTheory.attack(attackerHistoryW, defenderHistoryW);
            //Log::writeFormatLog(Trace, "Attacker Policy {} @ Hist {} vs. Defender Policy {} @ Hist {}. Result={}", sourceSol, attackerHistoryW.toString(), targetSol, defenderHistoryW.toString(), result);
            if (result==1) {
                // There is an attack!
                findAttack=true;
                for (auto i = def_place; i < defHistories.size(); ++i) {
                    a.addEdge(attIdx, defHistories[i], histories.at(targetSol).at(defHistories[i])->probability);
                    //Log::writeFormatLog(Debug, "***Attacker Policy {} @ Hist {} ATTACKS Defender Policy {} @ Hist {} with Pr={}", sourceSol, attackerHistoryW.toString(), targetSol, defenderHistoryW.toString(), histories.at(targetSol).at(defHistories[i])->probability);
                }
                defHistories.resize(def_place);
                break;
            }
        }
        if (findAttack) { break; }
    }
    return a;
}