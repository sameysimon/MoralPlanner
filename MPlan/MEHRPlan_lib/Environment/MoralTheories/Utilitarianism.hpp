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
#include "HistoryHandler.hpp"
#include <cmath>
#include <iostream>


class ExpectedUtility : public WorthBase {
public:
    double value=0;

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
    [[nodiscard]] unique_ptr<WorthBase> clone() const override {
        return make_unique<ExpectedUtility>(*this);
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
protected:
    std::unordered_map<Successor*, ExpectedUtility*> mJudgementMap;
    std::vector<double> mHeuristicList;

    static ExpectedUtility& quickCast(WorthBase& w) {
        return static_cast<ExpectedUtility&>(w);
    }
public:
    Utilitarianism() = default;
    Utilitarianism(json &t, size_t id) : Consideration(id) {
        label = t["Name"];
        // Process heuristics
        for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->mHeuristicList.push_back(it.value());
        }
    }
    explicit Utilitarianism(int id_) : Consideration(id_) {
        mJudgementMap = std::unordered_map<Successor*, ExpectedUtility*>();
    }
    //
    // Getters
    //
    WorthBase* judge(Successor& successor) override {
        return mJudgementMap[&successor];
    }
    unique_ptr<WorthBase> gather(const std::vector<WorthBase*>& worth, const std::vector<double>& probs, const std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        double utility = 0;
        ExpectedUtility* ex;
        for (int i = 0; i < worth.size(); i++) {
            auto j = static_cast<ExpectedUtility*>(worth[i]);
            ex = static_cast<ExpectedUtility*>(baselines[i]);
            double newVal = j->value + ex->value;
            if (!ignoreProbability) {
                newVal *= probs[i];
            }
            utility+=newVal;
        }
        auto res = make_unique<ExpectedUtility>(utility);
        return res;
    }

    std::vector<double> normalise(std::vector<WorthBase*> &worth_vec) override {
        std::vector<double> r;
        auto max_worth = std::max_element(worth_vec.begin(), worth_vec.end(), [](auto a, auto b) {
            return a->compare(*b) == 1;
        });
        auto min_worth = std::min_element(worth_vec.begin(), worth_vec.end(), [&](auto a, auto b) {
            return a->compare(*b) == 1;
        });
        auto max_util = quickCast(**max_worth.base()).value;
        auto min_util = quickCast(**min_worth.base()).value;
        for (auto wb : worth_vec) {
            r.push_back( (quickCast(*wb).value - min_util) / (max_util - min_util) );
        }
        return r;
    }

    std::unique_ptr<WorthBase> newHeuristic(State& s) override {
        if (s.id > mHeuristicList.size() || mHeuristicList.empty()) {
            return UniqueWorth();
        }
        return make_unique<ExpectedUtility>(mHeuristicList[s.id]);
    }
    WorthBase* newWorth() override {
        return new ExpectedUtility();
    }
    std::unique_ptr<WorthBase> UniqueWorth() override {
        return std::make_unique<ExpectedUtility>();
    }
    //
    // Initialisation
    //
    void processSuccessor(Successor* successor, json &successorData) override {
        double val = successorData;
        auto u = new ExpectedUtility();
        u->value = val;
        this->mJudgementMap.insert(std::make_pair(successor, u));
    }
};

class MEHRUtilitarianism : public MEHRTheory {
    vector<size_t> considerationIndex;
    unique_ptr<SortHistories> pSortedHistories;
public:
    MEHRUtilitarianism(size_t rank_, size_t theory_id, std::string &name_) : MEHRTheory(rank_, theory_id, name_) {
        pSortedHistories = make_unique<SortHistories>(*this);
    }
    int attack(QValue& qv1, QValue& qv2) override;
    Attack CriticalQuestionOne(Attack& att, policy_hists& histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(policy_hists &histories) override {
        pSortedHistories->InitMEHR(histories);
    }
    void AddPoliciesForMEHR(policy_hists &histories) override {
        pSortedHistories->AddPolicyHistories(histories);
    }
    SortHistories* getSortedHistories() {
        return pSortedHistories.get();
    }
    void AddConsideration(Consideration& con) override {
        considerationIndex.push_back(con.id);
    };
};

class MiniUtilitarianism: public Utilitarianism {
public:
    MiniUtilitarianism() = default;
    MiniUtilitarianism(json &t, size_t id) : Utilitarianism(t, id) {}
    unique_ptr<WorthBase> gather(const std::vector<WorthBase*>& worth, const std::vector<double>& probs, const std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        double utility = 0;
        double minUtility = 0;
        ExpectedUtility* ex;
        for (int i = 0; i < worth.size(); i++) {
            auto j = static_cast<ExpectedUtility*>(worth[i]);
            ex = static_cast<ExpectedUtility*>(baselines[i]);
            double newVal = j->value + ex->value;
            utility+=newVal;
            minUtility = std::min(utility, minUtility);
        }
        auto res = make_unique<ExpectedUtility>(minUtility);
        return res;
    };
};
class MaxiUtilitarianism: public Utilitarianism {
public:
    MaxiUtilitarianism() = default;
    MaxiUtilitarianism(json &t, size_t id) : Utilitarianism(t, id) {}
    unique_ptr<WorthBase> gather(const std::vector<WorthBase*>& worth, const std::vector<double>& probs, const std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        double utility = 0;
        double maxUtility = 0;
        ExpectedUtility* ex;
        for (int i = 0; i < worth.size(); i++) {
            auto j = static_cast<ExpectedUtility*>(worth[i]);
            ex = static_cast<ExpectedUtility*>(baselines[i]);
            double newVal = j->value + ex->value;
            utility+=newVal;
            maxUtility = std::max(utility, maxUtility);
        }
        auto res = make_unique<ExpectedUtility>(maxUtility);
        return res;
    };
};