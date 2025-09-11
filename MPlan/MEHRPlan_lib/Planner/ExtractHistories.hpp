//
// Created by Simon Kolker on 24/10/2024.
//

#pragma once
#include "iostream"
#include "Policy.hpp"

class History {
public:
    QValue worth;
    mutable double probability;
    bool hasPath;
    vector<Successor*>* path = nullptr;
    History(const History& hist) {
        worth = hist.worth;
        probability = hist.probability;
        hasPath = hist.hasPath;
        if (hist.hasPath && hist.path!=nullptr) {
            path = new vector<Successor*>(hist.path->size());
            for (size_t i = 0; i < hist.path->size(); ++i) {
                (*path)[i] = hist.path->at(i);
            }
        }
    }

    explicit History(QValue& _worth, double _probability=1, bool usePath=false) : worth(_worth), probability(_probability), hasPath(usePath) {
        if (hasPath) {
            path = new vector<Successor*>();
        }
    }
    ~History() {
        delete path;
    }
    bool operator==(const History& other) const {
        return worth == other.worth;
    }
    void addToPath(Successor* successor) {
        if (!hasPath) {
            return;
        }
        path->push_back(successor);
    }

};

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
        return *lhs == *rhs;
    }
};

class ExtractHistories {
public:
    explicit ExtractHistories(MDP& mdp) : mdp(mdp) { }

    //
    // Entry Point/Main Call.
    //
    void extract(vector<vector<History*>>& histories, vector<Policy*>& policySet) {
        //
        // Extract information from solution set.
        //
        // Stores set of histories against policy's index.
        vector<unordered_set<History*, HistoryPtrHash, HistoryPtrEqual>> piToHSet;
        for (Policy* pi : policySet) {
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

    static string ToString(vector<Policy*>& policySet, vector<QValue>& solExpectations, vector<vector<History*>>& histories) {
        stringstream ss;
        for (int solIdx=0; solIdx < policySet.size(); solIdx++) {
            double total = 0;
            ss << "Policy " << solIdx << " expects " << solExpectations[solIdx].toString() << std::endl;
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
        auto stateActionIt = pi.policy.find(state.id);
        if (stateActionIt==pi.policy.end()) {
            DFS_baseCase(h, hSet);
            return;// Returning early means history h not deleted.
        }
        auto successors = MDP::getActionSuccessors(state, stateActionIt->second);
        // Recursive Case.
        for (Successor* successor : *successors) {
            auto* h_ = new History(*h);
            // TODO smarter, copying other histories thing? Reduce number of copied sequences.
            mdp.addCertainSuccessorToQValue(h_->worth, successor);
            h_->probability *= successor->probability;
            h_->addToPath(successor);
            DFS_Histories(*mdp.states[successor->target], time+1, pi, h_, hSet);
        }

        delete h;
    }

    void extractHistoriesIterative(Policy& pi) {

    }

};



