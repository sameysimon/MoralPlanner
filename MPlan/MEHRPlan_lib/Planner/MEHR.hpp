//
// Created by Simon Kolker on 30/08/2024.
//
#ifndef MEHR_H
#define MEHR_H
#include "Solution.hpp"
#include "AttackType.hpp"
#include "iostream"
#include "ExtractHistories.hpp"
#include <unordered_set>

using namespace std;

struct Attack {
    int sourcePolicyIdx;
    int targetPolicyIdx;
    int theoryIdx;
    int sourceHistoryIdx;
    int targetHistoryIdx;
    Attack(int sourcePolicyIdx, int targetPolicyIdx, int theoryIdx, int sourceHistoryIdx, int targetHistoryIdx) :
    sourcePolicyIdx(sourcePolicyIdx), targetPolicyIdx(targetPolicyIdx), theoryIdx(theoryIdx), sourceHistoryIdx(sourceHistoryIdx), targetHistoryIdx(targetHistoryIdx) {}
};

class MEHR {
    MDP& mdp;
    vector<double>* non_accept;
    vector<vector<Attack>*>* attacksByTarget;
  public:
    MEHR(MDP& mdp) : mdp(mdp) { }
    ~MEHR() {
        delete non_accept;
        for (auto attacks : *attacksByTarget) {
            delete attacks;
        }
        delete attacksByTarget;
    };
    vector<double>* findNonAccept(vector<QValue>& policyWorths, vector<vector<History*>>& histories);
    void checkForAttack(int sourceSol, int targetSol, vector<vector<History*>>& histories, vector<int>& theories);

    string ToString(vector<QValue>& policyWorths, vector<vector<History*>>& histories) {
        stringstream ss;
        if (attacksByTarget==nullptr) {
            ss << "Uninitialised.";
            return ss.str();
        }
        for (int tarIdx=0; tarIdx<attacksByTarget->size(); tarIdx++) {
            ss << " Attacks on policy " << tarIdx << " with expected worth " << policyWorths[tarIdx].toString() << "." << endl;
            for (Attack& att : *(*attacksByTarget)[tarIdx]) {
                History* targetHist = histories.at(att.targetPolicyIdx).at(att.targetHistoryIdx);
                History* sourceHist = histories.at(att.sourcePolicyIdx).at(att.sourceHistoryIdx);
                ss << "     From Pi " << att.sourcePolicyIdx << ", history " << att.sourceHistoryIdx << "(" << sourceHist->worth.toString() <<") ---> " << att.targetHistoryIdx << " ("<< targetHist->worth.toString() << ") with P= +" << targetHist->probability << " by theory " << att.theoryIdx << endl;
            }
            ss << " Pi " << tarIdx << " non-Acceptability = " << non_accept->at(tarIdx) << endl << endl;;
        }
        return ss.str();
    }


};



#endif //MEHR_H
