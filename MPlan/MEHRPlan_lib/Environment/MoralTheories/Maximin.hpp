//
// Created by Simon Kolker on 23/04/2025.
//
#pragma once
#include "MoralTheory.hpp"
#include "Successor.hpp"
#include "State.hpp"
#include "Utilitarianism.hpp"
#include "unordered_set"
#include <sstream>


class Maximin : public Consideration {
    std::vector<double> heuristicList;
    std::unordered_map<Successor*, ExpectedUtility*> judgementMap;
public:
    bool sortHistories = false;
    std::string theoryName;
    Maximin(json &t, size_t id_) : Consideration(id_) {
        label = t["Name"];
        rank = t["Rank"];
        for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->heuristicList.push_back(it.value());
        }
    }

    ExpectedUtility* judge(Successor& successor) {
        return judgementMap[&successor];
    }

    WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        double utility = 0;
        ExpectedUtility* ex;
        for (int i = 0; i < successors.size(); i++) {
            ExpectedUtility* j = judge(*successors[i]);
            ex = static_cast<ExpectedUtility*>(baselines[i]);
            double newVal = j->value + ex->value;
            if (!ignoreProbability) {
                newVal *= successors[i]->probability;
            }
            utility+=newVal;
        }

        ex = new ExpectedUtility();
        ex->value = utility;
        return ex;
    };

    WorthBase* newHeuristic(State& s) override {
        auto eu = new ExpectedUtility();
        if (s.id > heuristicList.size()) {
            throw std::runtime_error("Heuristic list is too small");
        }
        eu->value = heuristicList[s.id];
        return eu;
    };
    WorthBase* newWorth() override {
        return new ExpectedUtility();
    }
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        double val = successorData;
        auto u = new ExpectedUtility();
        u->value = val;
        this->judgementMap.insert(std::make_pair(successor, u));
    }


};
class MEHRMaximin : public MEHRTheory {
    std::vector<size_t> considerations;
    // Stores attacked histories for each policy. attacks[policy_idx] = [attacked histories]
    std::vector<std::unordered_set<size_t>> attacks;
public:
    MEHRMaximin(size_t rank_, size_t id_, std::string &name_) : MEHRTheory(rank_, id_, name_) { }
    void addConsideration(size_t c_idx) { considerations.push_back(c_idx); }
    int attack(QValue& qv1, QValue& qv2) override;
    Attack CriticalQuestionOne(size_t sourceSol, size_t targetSol, std::vector<std::vector<History*>> &histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<History*>> &histories) override;
    void AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) override;
};