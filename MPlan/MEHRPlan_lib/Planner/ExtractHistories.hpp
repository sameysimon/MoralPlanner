//
// Created by Simon Kolker on 24/10/2024.
//

#pragma once
#include "iostream"
#include "Logger.hpp"
#include "Policy.hpp"
#include "History.hpp"

using namespace std;

class HistoryPtrHash {
public:
    std::size_t operator()(const History* h) const noexcept {
        QValueHash qValHash;
        return qValHash(h->worth);
    };
};

class HistoryPtrEqual {
public:
    bool operator()(const History* lhs, const History* rhs) const noexcept {
        return lhs->worth == rhs->worth;
    }
};

class ExtractHistories {

public:
    explicit ExtractHistories(MDP& mdp) : mdp(mdp) { }

    //
    // Entry Point/Main Call.
    //
    void extract(vector<vector<History*>>& histories, vector<unique_ptr<Policy>>& policySet) {
        //
        // Extract information from solution set.
        //
        // Stores set of histories against policy's index.
        vector<unordered_set<History*, HistoryPtrHash, HistoryPtrEqual>> piToHSet;
        for (auto& pi : policySet) {
            // Add/Extract History outcomes
            std::string x = pi->worth[0].toString();
            auto hSet = extractHistories(*pi);
            piToHSet.push_back(std::move(*hSet));
            delete hSet;
        }

        histories.clear();
        for (const auto& hSet : piToHSet) {
            // Create a vector to store this policy's histories
            std::vector hVec(hSet.begin(), hSet.end());
            histories.push_back(std::move(hVec));
        }

    }

    void ReduceToUniquePolicies(vector<unique_ptr<Policy>>& policySet, vector<vector<History*>>& histories) {

        class PolicyHash {
            const std::vector<std::vector<History*>>& histories;
            public:
            explicit PolicyHash(const std::vector<std::vector<History*>>& h) : histories(h) {}
            std::size_t operator()(const size_t piIdx) const noexcept {
                QValueHash qValHash;
                std::size_t hash = 0;
                for (auto hist : histories[piIdx]) {
                    QValue::hash_combine(hash, qValHash(hist->worth));
                    int rounded = (int)(hist->probability*1000.0);
                    QValue::hash_combine(hash, std::hash<int>()(rounded));
                }
                Log::writeFormatLog(LogLevel::Trace, "Policy hashed to {}", hash);
                return hash;
            };
        };
        class PolicyEqual {
            const std::vector<std::vector<History*>>& histories;
        public:
            explicit PolicyEqual(const std::vector<std::vector<History*>>& h) : histories(h) {}
            bool operator()(const size_t lhs, const size_t rhs) const noexcept {
                if (histories[lhs].size() != histories[rhs].size()) return false;
                for (size_t i = 0; i < histories[lhs].size(); ++i) {
                    bool found = false;
                    for (size_t j = 0; j < histories[rhs].size(); ++j) {
                        if (histories[lhs][i]->isEquivalent(*histories[rhs][j])) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        return false;
                    }
                }
                return true;
            }
        };


        // Set of policy ids unique with respect to set of histories.
        unordered_set<size_t, PolicyHash, PolicyEqual> uniquePolicies(policySet.size() * 2, PolicyHash(histories), PolicyEqual(histories));
        size_t uniquePoliciesCount = 0;
        for (size_t i = 0; i < policySet.size(); ++i) {
            uniquePolicies.insert(i);
            if (uniquePolicies.size() > uniquePoliciesCount) {
                uniquePoliciesCount++;
            }

        }

        // Move back to the vector.
        vector<unique_ptr<Policy>> policySet_(uniquePolicies.size());
        vector<vector<History*>> historySet_(uniquePolicies.size());
        size_t i = 0;
        for (size_t pi_idx : uniquePolicies) {
            policySet_[i] = std::move(policySet[pi_idx]);
            historySet_[i] = std::move(histories[pi_idx]);
            i++;
        }
        histories = std::move(historySet_);
        policySet = std::move(policySet_);

    }



    static string ToString(vector<unique_ptr<Policy>>& policySet, vector<vector<History*>>& histories) {
        stringstream ss;
        for (int solIdx=0; solIdx < policySet.size(); solIdx++) {
            double total = 0;
            ss << "Policy " << solIdx << " expects " << policySet[solIdx]->worth[0].toString() << std::endl;
            int histCounter=0;
            for (History* hist : histories[solIdx]) {
                ss << "      History #" << histCounter << " has " << hist->worth.toString() << " at Probability " <<  hist->probability << std::endl;
                total += hist->probability;
                histCounter++;
            }
            ss << "      Total Probability = " << total << std::endl;
        }
        ss << std::endl;
        return ss.str();
    }


private:
    MDP& mdp;
    int counter = 0;
    unordered_set<History*, HistoryPtrHash, HistoryPtrEqual>* extractHistories(Policy& pi) {
        QValue qval = QValue();
        mdp.blankQValue(qval);
        // TODO Some better mechanism to determine if we want to keep history?
        auto firstHistory = new History(qval, 1, true);
        auto hSet = new unordered_set<History*, HistoryPtrHash, HistoryPtrEqual>();
        DFS_Histories(*mdp.states[0], 0, pi, firstHistory, hSet);
        return hSet;
    }
    //
    // Depth First Search through Policy
    //
    void DFS_baseCase(History* h, unordered_set<History*, HistoryPtrHash, HistoryPtrEqual>* hSet) {
        auto hIter = hSet->find(h);
        if (hIter == hSet->end())
            hSet->insert(h);
        else
            (*hIter)->probability += h->probability;
    }

    void DFS_Histories(State& state, int time, Policy& pi, History* h, unordered_set<History*, HistoryPtrHash, HistoryPtrEqual>* hSet) {
        // Base Case. Stop when reaching the horizon.
        if (time >= mdp.horizon) {
            DFS_baseCase(h, hSet);
            return;// Returning early means history h not deleted.
        }
        // Stop if state/time is not in the policy
        auto stateActionIt = pi.getAction(state.id);
        if (stateActionIt==-1) {
            DFS_baseCase(h, hSet);
            return;// Returning early means history h not deleted.
        }
        auto successors = MDP::getActionSuccessors(state, stateActionIt);
        // Recursive Case.
        for (Successor* successor : *successors) {
            auto* h_ = new History(*h);
            // TODO smarter, copying other histories thing? Reduce number of copied sequences.
            mdp.addCertainSuccessorToQValue(h_->worth, successor);
            h_->probability *= successor->probability;
            // At low probabilities, math gets very weird. Think this is fine...
            // Treat these as dead-ends
            h_->addToPath(successor);
            DFS_Histories(*mdp.states[successor->target], time+1, pi, h_, hSet);
        }

        delete h;
    }

    void extractHistoriesIterative(Policy& pi) {

    }

};



