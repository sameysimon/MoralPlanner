//
// Created by Simon Kolker on 02/06/2026.
//

#include "Planner/Solver.hpp"

std::vector<int> Solver::Pprune(MDP& mdp, std::vector<QValue>& inVector) {
    std::vector<int> result;
    Pprune(mdp, inVector, result);
    return result;
}
void Solver::Pprune(MDP& mdp, std::vector<QValue>& inVector, std::vector<int>& outVector) {
    Pprune(mdp, inVector, outVector, [](const QValue& q) -> const QValue& {
        return q;
    });
}
template <typename T, typename Accessor>
std::vector<int> Solver::Pprune(MDP& mdp, const T& inVector, Accessor getQValue) {
    std::vector<int> outVector;
    Pprune(mdp, inVector, outVector, getQValue);
    return outVector;
}

template <typename T, typename Accessor>
void Solver::Pprune(MDP& mdp, const T& inVector, std::vector<int>& outVector, Accessor getQValue) {
    if (inVector.size() == 0) { return; }

    std::vector<bool> inBudget;
    bool anyInBudget = false;

    if (mdp.non_moralTheoryIdx != -1) {
        for (int i = 0; i < inVector.size(); i++) {
            inBudget.push_back(mdp.isQValueInBudget(getQValue(inVector[i])));
            if (inBudget[i]) {
                anyInBudget = true;
            }
        }
    }

    for (int i = 0; i < inVector.size(); i++) {
        if (anyInBudget && !inBudget[i]) {
            continue; // Over budget QValues cannot be undominated/added to outVector
        }
        auto& qv = getQValue(inVector[i]);
        bool isDominated = false;
        for (int j = 0; j < inVector.size(); j++) {
            if (j == i) continue;
            if (anyInBudget && !inBudget[j]) {
                continue; // Over budget QValues cannot dominate anything.
            }
            int r = mdp.ParetoCompare(qv, getQValue(inVector[j]));
            if (r == -1) {
                isDominated = true;
                break;
            }
        }
        if (!isDominated) {
            outVector.push_back(i);
        }
    }
}

