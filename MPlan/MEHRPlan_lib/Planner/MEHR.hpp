//
// Created by Simon Kolker on 30/08/2024.
//
#ifndef MEHR_H
#define MEHR_H
#include "Solution.hpp"
#include "AttackType.hpp"
#include "iostream"
#include "ExtractHistories.hpp"
#include <numeric>
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
#ifdef DEBUG                
    long long search_time = 0;
    long long attack_time = 0;
    long long total_comp_policy_exps = 0;
    long long hist_exps = 0;
    long long clear_time = 0;
    long long total_CFA = 0;
#endif

    MDP& mdp;
    vector<double>* non_accept;
    vector<vector<Attack>>* attacksByTarget;
    
    vector<vector<vector<int>>>* historiesRankedTheory; // hRT[theoryIdx][actionIdx] = [histIdx1, histIdx2, ...]
    void sortHistories(vector<vector<History*>>& histories) {
        /*size_t numOfPolicies = histories.size();
        historiesRankedTheory = new vector<vector<vector<int>>>;
        for (int tIdx = 0; tIdx < mdp.theories.size(); tIdx++) {
            auto x = vector(numOfPolicies, vector<int>());
            historiesRankedTheory[tIdx].emplace_back(x);
            for (int pIdx = 0; pIdx < numOfPolicies; pIdx++) {
                iota(historiesRankedTheory[tIdx][pIdx].begin(), historiesRankedTheory[tIdx][pIdx].end(), 1);
                sort(historiesRankedTheory[tIdx][pIdx].begin(), historiesRankedTheory[tIdx][pIdx].end(),
                    [histories, pIdx, tIdx](const int& lhs, const int& rhs) {
                        // Compare histories[pIdx][lhs].worth.expectations[tIdx] and histories[pIdx][rhs].worth.expectations[tIdx]
                        int r = histories[pIdx][lhs]->worth.expectations[tIdx]->compare(*histories[pIdx][rhs]->worth.expectations[tIdx]);
                        return r==1;// ascending order. r=1 means lhs beats rhs. Means lhs goes before.
                    });
            }
        }*/
    }



  public:
    MEHR(MDP& mdp) : mdp(mdp) { }
    ~MEHR() {
        delete non_accept;
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
            for (Attack& att : (*attacksByTarget)[tarIdx]) {
                History* targetHist = histories.at(att.targetPolicyIdx).at(att.targetHistoryIdx);
                History* sourceHist = histories.at(att.sourcePolicyIdx).at(att.sourceHistoryIdx);
                ss << "     From Pi " << att.sourcePolicyIdx << ", history " << att.sourceHistoryIdx << "(" << sourceHist->worth.toString() <<") ---> " << att.targetHistoryIdx << " ("<< targetHist->worth.toString() << ") with P= +" << targetHist->probability << " by theory " << att.theoryIdx << "(" << mdp.theories[att.theoryIdx]->label << ")" << endl;
            }
            ss << " Pi " << tarIdx << " non-Acceptability = " << non_accept->at(tarIdx) << endl << endl;;
        }
        return ss.str();
    }


};



#endif //MEHR_H
