//
// Created by Simon Kolker on 30/08/2024.
//
#ifndef MEHR_H
#define MEHR_H
#include "AttackType.hpp"
#include "iostream"
#include "ExtractHistories.hpp"
#include <numeric>
#include "../Logger.hpp"
#include <unordered_set>
#include <list>

using namespace std;

struct AttackTargetHash {
    std::size_t operator()(const std::array<int, 3>& arr) const {
        std::size_t h1 = std::hash<int>()(arr[0]);
        std::size_t h2 = std::hash<int>()(arr[1]);
        std::size_t h3 = std::hash<int>()(arr[2]);
        return h1 ^ (h2 << 1) ^ (h3 << 2); // Simple combination
    }
};

struct Attack {
    int sourcePolicyIdx;
    int targetPolicyIdx;
    int theoryIdx;
    int sourceHistoryIdx;
    int targetHistoryIdx;
    double probability;
    Attack(int sourcePolicyIdx, int targetPolicyIdx, int theoryIdx, int sourceHistoryIdx, int targetHistoryIdx, double p) :
    sourcePolicyIdx(sourcePolicyIdx), targetPolicyIdx(targetPolicyIdx), theoryIdx(theoryIdx), sourceHistoryIdx(sourceHistoryIdx), targetHistoryIdx(targetHistoryIdx), probability(p) {}
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
    bool useAttackHash=true;
    MDP& mdp;
    vector<QValue>& policyWorths;
    vector<vector<History*>>& histories;
    // Stores a vector of Attacks on each policy: attacksByTarget[i] = [attacks on policy with idx i]
    unordered_set<array<int, 3>, AttackTargetHash> attacks;

    // Builds hRT (histories' Ranked by Theories)
    // For each theory, each policy, all histories in order.
    // Sort costs HlogH (for H histories). Thus, T*PI*H*logH.
    // This can reduce
    // hRT[theoryIdx][piIdx] = [histIdx1, histIdx2, ...
    void addAttack(int sourcePolicyIdx, int targetPolicyIdx, int theoryIdx, int sourceHistoryIdx, int targetHistoryIdx, double prob) {
        attacks.insert({targetPolicyIdx, targetHistoryIdx, theoryIdx});
        (*attacksByTarget)[targetPolicyIdx].emplace_back(sourcePolicyIdx, targetPolicyIdx, theoryIdx, sourceHistoryIdx, targetHistoryIdx, prob);
    }

    bool checkAttackExists(int policyIdx, int historyIdx, int theoryIdx) {
        if (useAttackHash) {
            return attacks.contains({policyIdx, historyIdx, theoryIdx});
        }
        auto& potAttacks = (*attacksByTarget)[policyIdx];
        for (Attack& att : potAttacks) {
            // Looping through all attacks to make this check is naive, though only 0.18% of MEHR computation time currently.
            if (att.targetHistoryIdx==historyIdx && att.theoryIdx==theoryIdx) {
                return true;
            }
        }
        return false;
    }



  public:
    vector<vector<Attack>>* attacksByTarget;
    MEHR(MDP& mdp, vector<vector<History*>>& histories_, vector<QValue>& policyWorths_, bool useAttackHash_)
    : mdp(mdp), histories(histories_), policyWorths(policyWorths_), useAttackHash(useAttackHash_) {
        attacksByTarget = new vector<vector<Attack>>();
        for (int i = 0; i < policyWorths.size(); i++) {
            vector<Attack> attacksOnPolicy = vector<Attack>();
            attacksByTarget->push_back(attacksOnPolicy);
        }

        // Prepare each moral theory for MEHR
        for (auto t : mdp.mehr_theories) {
            t->InitMEHR(histories);
        }

    }

    ~MEHR() {
        cout << "MEHR DESTRCUTOR" << endl;
        delete attacksByTarget;
    };
    void SortHistories(vector<vector<History*>>& histories, vector<vector<vector<int>>>& hRT) const;
    void findNonAccept(vector<double> &non_accept);
    double checkForAttack(int sourceSol, int targetSol, vector<int>& mehrTheories);

    string ToString(vector<QValue>& policyWorths, vector<vector<History*>>& histories, vector<double>& non_accept) {
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
                ss << "     From Pi " << att.sourcePolicyIdx << ", history " << att.sourceHistoryIdx << "(" << sourceHist->worth.toString() <<") ---> " << att.targetHistoryIdx << " ("<< targetHist->worth.toString() << ") with P= +" << targetHist->probability << " by theory " << att.theoryIdx << "(" << mdp.considerations[att.theoryIdx]->label << ")" << endl;
            }
            ss << " Pi " << tarIdx << " non-Acceptability = " << non_accept.at(tarIdx) << endl << endl;;
        }
        return ss.str();
    }


};



#endif //MEHR_H
