//
// Created by Simon Kolker on 01/10/2024.
//

#ifndef THRESHOLD_HPP
#define THRESHOLD_HPP

#include "MoralTheory.hpp"
#include "Successor.hpp"
#include "State.hpp"
#include <cmath>
#include <sstream>

#include "Utilitarianism.hpp"

// The Morally Relevant Information
class ExpectedValue : public WorthBase {
public:
    double value=0;
    // Use simple numeric operators
    int compare(WorthBase& wb) const override {
        ExpectedValue* eu = static_cast<ExpectedValue*>(&wb);
        if (value < eu->value)
            return -1;
        if (value > eu->value)
            return 1;
        return 0;
    }
    std::string ToString() const override {
        return doubleToString(value);
    }
    bool isEquivalent(WorthBase& w) const override {
        ExpectedValue* eu = dynamic_cast<ExpectedValue*>(&w);
        if (eu==nullptr) {
            throw std::invalid_argument("Expected WorthBase to be of type ExpectedValue");
            return false;
        }
        return (std::abs(value - eu->value) < 1e-3);
    }
    WorthBase* clone() const override {
        return new ExpectedValue(*this);
    }
    ExpectedValue() {value=0;}
    ExpectedValue(const ExpectedValue& other) {
        this->value = other.value;
    }
    ~ExpectedValue() = default;
    ExpectedValue& operator=(WorthBase& w) {
        if (const ExpectedValue* eu = dynamic_cast<const ExpectedValue*>(&w)) {
            this->value = eu->value;
        }
        return *this;
    }
    std::size_t hash() override {
        return std::hash<double>()(value);
    }
};




// The Moral Consideration
class Threshold : public Consideration {
    std::unordered_map<Successor*, ExpectedValue*> judgementMap;
    std::vector<double> heuristicList;

    static ExpectedValue& quickCast(WorthBase& w) {
        return static_cast<ExpectedValue&>(w);
    }
public:
    Threshold(size_t id) : Consideration(id) {
        judgementMap = std::unordered_map<Successor*, ExpectedValue*>();
    }
    Threshold(json& t, size_t id) : Consideration(id) {
        label = t["Name"];
        rank = t["Rank"];
        judgementMap = std::unordered_map<Successor*, ExpectedValue*>();
        for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->heuristicList.push_back(it.value());
        }

    }
    //
    // Getters
    //
    ExpectedValue* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability=false) override {
        double utility = 0;
        ExpectedValue* ex;
        for (int i = 0; i < successors.size(); i++) {
            ExpectedValue* j = judge(*successors[i]);
            ex = static_cast<ExpectedValue*>(baselines[i]);// May be a better way to do this?
            double newVal = j->value + ex->value;
            if (not ignoreProbability) {
                newVal *= successors[i]->probability;
            }
            utility+=newVal;
        }

        ex = new ExpectedValue();
        ex->value = utility;
        return ex;
    };
    WorthBase* newHeuristic(State& s) override {
        ExpectedValue* eu = new ExpectedValue();
        eu->value = heuristicList[s.id];
        return eu;
    };
    WorthBase* newWorth() override {
        return new ExpectedValue();
    }
    //
    // Initialisation
    //
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        double val = successorData;
        auto u = new ExpectedValue();
        u->value = val;
        this->judgementMap.insert(std::make_pair(successor, u));
    }
};

class MEHRThreshold : public MEHRTheory {
    size_t considerationIdx;
    double threshold;
    SortHistories *pSortedHistories;
public:
    explicit MEHRThreshold(size_t rank_, size_t conId, size_t theoryID, double threshold, std::string &name_) : MEHRTheory(rank_, theoryID, name_), considerationIdx(conId), threshold(threshold) {
        pSortedHistories = new SortHistories(*this);
    }
    int attack(QValue& qv1, QValue& qv2) override;
    Attack CriticalQuestionOne(size_t sourceSol, size_t targetSol, std::vector<std::vector<History*>> &histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->InitMEHR(histories);
    }
    void AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->AddPolicyHistories(histories);
    }

};



#endif //THRESHOLD_HPP
