//
//  Utilitarianism.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#pragma once

#include "MoralTheory.hpp"
#include "Successor.hpp"
#include "State.hpp"
#include <cmath>
#include <iostream>


class ExpectedUtility : public WorthBase {
public:
    double value=0;
    // Use simple numeric operators
    int compare(WorthBase& wb) const override {
        auto eu = static_cast<ExpectedUtility*>(&wb);
        if (value < eu->value)
            return -1;
        if (value > eu->value)
            return 1;
        return 0;
    }
    [[nodiscard]] std::string ToString() const override {
        return doubleToString(value);
    }
    bool isEquivalent(WorthBase& w) const override {
        ExpectedUtility* eu = dynamic_cast<ExpectedUtility*>(&w);
        if (eu==nullptr) {
            throw std::invalid_argument("Expected WorthBase to be of type ExpectedUtility");
            return false;
        }
        return (std::abs(value - eu->value) < 1e-3);
    }
    [[nodiscard]] WorthBase* clone() const override {
        return new ExpectedUtility(*this);
    }
    ExpectedUtility() {value=0;}
    explicit ExpectedUtility(double v) {value=v;}
    ExpectedUtility(const ExpectedUtility& other)  : WorthBase(other) {
        this->value = other.value;
    }
    ~ExpectedUtility() override = default;
    ExpectedUtility& operator=(WorthBase& w) {
        if (const ExpectedUtility* eu = dynamic_cast<const ExpectedUtility*>(&w)) {
            this->value = eu->value;
        }
        return *this;
    }
    std::size_t hash() override {
        return std::hash<double>()(value);
    }
};


class Utilitarianism : public Consideration {
    std::unordered_map<Successor*, ExpectedUtility*> judgementMap;
    std::vector<double> heuristicList;

    static ExpectedUtility& quickCast(WorthBase& w) {
        return static_cast<ExpectedUtility&>(w);
    }
public:
    Utilitarianism(json &t, size_t id) : Consideration(id) {
        label = t["Name"];
        // Process heuristics
        for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->heuristicList.push_back(it.value());
        }
    }
    explicit Utilitarianism(int id_) : Consideration(id_) {
        judgementMap = std::unordered_map<Successor*, ExpectedUtility*>();
    }
    //
    // Getters
    //
    ExpectedUtility* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability=false) override {
        double utility = 0;
        ExpectedUtility* ex;
        for (int i = 0; i < successors.size(); i++) {
            ExpectedUtility* j = judge(*successors[i]);
            ex = static_cast<ExpectedUtility*>(baselines[i]);
            double newVal = j->value + ex->value;
            if (not ignoreProbability) {
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

    //
    // Initialisation
    //
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        double val = successorData;
        auto u = new ExpectedUtility();
        u->value = val;
        this->judgementMap.insert(std::make_pair(successor, u));
    }
};

class MEHRUtilitarianism : public MEHRTheory {
    size_t considerationIdx;
    SortHistories *pSortedHistories;
public:
    MEHRUtilitarianism(size_t rank_, size_t id, std::string &name_) : MEHRTheory(rank_, name_), considerationIdx(id) {
        pSortedHistories = new SortHistories(*this);
    }
    /*~MEHRUtilitarianism() override {
     *Not sure if I need this to get rid of pSortedHistories, if it will deal with base class stuff
        delete pSortedHistories;
    }*/
    int attack(QValue& qv1, QValue& qv2) override;
    double CriticalQuestionOne(int sourceSol, int targetSol, std::vector<std::vector<History*>>& histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->InitMEHR(histories);
    }
    SortHistories* getSortedHistories() {
        return pSortedHistories;
    }
};

