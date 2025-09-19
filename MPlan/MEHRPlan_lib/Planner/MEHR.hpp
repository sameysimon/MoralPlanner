//
// Created by Simon Kolker on 30/08/2024.
//
#ifndef MEHR_H
#define MEHR_H
#include "AttackType.hpp"
#include "iostream"
#include "MoralTheory.hpp"
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

struct NonAcceptability {
    // Indexed by theory-then policy. non_accept[tIdx][pIdx] = non_accept by theory tIdx for policy pIdx.
    std::vector<std::vector<double>> non_accept;
    NonAcceptability(size_t theoryCount, size_t policyCount) {
        non_accept.resize(theoryCount);
        for (vector<double> &n : non_accept) {
            n.resize(policyCount);
            std::fill(n.begin(), n.end(), 0);
        }
    }
    [[nodiscard]] double getPolicyNonAccept(size_t policyIdx) const {
        double r = 0;
        for (const vector<double>& n : non_accept) {
            r += n[policyIdx];
        }
        return r;
    }
    [[nodiscard]] size_t getTotalPolicies() {
        return non_accept[0].size();
    }
    [[nodiscard]] size_t getTotalTheories() const {
        return non_accept.size();
    }
    void addNonAccept(size_t theoryIdx, size_t policyIdx, double value) {
        non_accept[theoryIdx][policyIdx] += value;
    }
    void appendPolicy() {
        for (vector<double>& n : non_accept) {
            n.push_back(0);
        }
    }

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
    bool doneMEHR=false;
    bool useAttackHash=true;
    MDP& mdp;
    vector<Policy*>& policies;
    vector<vector<History*>>& histories;

    // Stores the best expected policies for each moral theory. Maps theory ID to vector of policy indices.
    unordered_map<size_t, vector<size_t>> bestExpectedPolicyByTheory;
    // Stores which policies are undominated at what ranks: invulnerablePolicies[3] = 2 -> policy Idx=3 is the best/undominated policy at ordinal rank 2.
    unordered_map<size_t, size_t> invulnerablePolicies;

  public:
    vector<vector<Attack>> attacks;
    MEHR(MDP& mdp, vector<Policy*> &policies_, vector<vector<History*>> &histories_)
    : mdp(mdp), policies(policies_), histories(histories_) {
        attacks.resize(policies.size());
        // Prepare each moral theory for MEHR based on these theories.
        for (auto t : mdp.mehr_theories) {
            t->InitMEHR(histories);
        }
    }

    // Normal MEHR Functions
    void findNonAccept(NonAcceptability &non_accept);
    int PolicyCompare(ushort theoryIdx, ushort rankIdx, const size_t &lhs, const size_t &rhs);
    int CheckAttackToRank(ushort toRank, const size_t& left, const size_t& right);
    vector<size_t> MaxPolicyForTheory(unsigned long& theoryIdx, size_t rank);
    void attackBetweenBestPolicies(NonAcceptability& non_accept,
                                   size_t rank, unsigned long& theoryIdx, vector<size_t>& bestPolicies);

    // Explainability functions
    void addPolicyToMEHR(Policy *pi, vector<History*> &h, NonAcceptability &non_accept);
    void addPoliciesToMEHR(NonAcceptability &non_accept, vector<Policy*> &newPolicies, vector<vector<History*>> &newHistories);
    void BestPoliciesAttackPi(NonAcceptability& non_accept, size_t rank, unsigned long& theoryIdx, vector<size_t>& bestPolicies, size_t pi_idx);
    void PiAttackBestPolicies(NonAcceptability& non_accept, size_t rank, unsigned long& theoryIdx, vector<size_t>& bestPolicies, size_t pi_idx);

    // Intepretation
    bool containsAttack(size_t sourcePolicy, size_t targetPolicy) {
        return std::any_of(attacks[targetPolicy].begin(),attacks[targetPolicy].end(),
            [sourcePolicy](Attack& a) {return a.sourcePolicyIdx==sourcePolicy;});
    }
    bool containsAttack(size_t sourcePolicy, size_t targetPolicy, const QValue& targetHistoryWorth) {
        for (auto &a : attacks[targetPolicy]) {
            if (a.sourcePolicyIdx==sourcePolicy) {
                for (std::pair<size_t,size_t> &ed : a.HistoryEdges) {
                    auto& tarWorth = histories[targetPolicy][ed.second]->worth;
                    if (tarWorth==targetHistoryWorth) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    bool containsAttack(size_t sourcePolicy, const QValue& sourceHistoryWorth, size_t targetPolicy, const QValue& targetHistoryWorth) {
        for (auto &a : attacks[targetPolicy]) {
            if (a.sourcePolicyIdx==sourcePolicy) {
                for (std::pair<size_t,size_t> &ed : a.HistoryEdges) {
                    auto& srcWorth = histories[sourcePolicy][ed.first]->worth;
                    auto& tarWorth = histories[targetPolicy][ed.second]->worth;
                    if (srcWorth==sourceHistoryWorth && tarWorth==targetHistoryWorth) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    string ToString(vector<QValue>& policyWorths, vector<vector<History*>>& histories, NonAcceptability& non_accept) {
        stringstream ss;
        for (int tarIdx=0; tarIdx < attacks.size(); tarIdx++) {
            ss << "Non-acceptability on policy " << tarIdx << " with expected worth " << policyWorths[tarIdx].toString() << " has non-accept " << non_accept.getPolicyNonAccept(tarIdx) << ":" << endl;
            for (Attack& att : attacks[tarIdx]) {
                ss << "   * From Policy " << att.sourcePolicyIdx << " (" << policyWorths[att.sourcePolicyIdx].toString() << ") by theory " << mdp.mehr_theories[att.theoryIdx]->mName << " P=" << att.p << endl;
                for (auto edge : att.HistoryEdges) {
                    ss << "     - History " << edge.first << "(" << histories[att.sourcePolicyIdx][edge.first]->worth.toString() << ") --> " << edge.second << " (" << histories[att.targetPolicyIdx][edge.second]->worth.toString() << ")" << " for P=" << histories[att.targetPolicyIdx][edge.second]->probability << endl;
                }
            }
        }
        return ss.str();
    }

};



#endif //MEHR_H
