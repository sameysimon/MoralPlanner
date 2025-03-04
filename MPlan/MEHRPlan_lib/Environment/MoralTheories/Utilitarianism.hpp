//
//  Utilitarianism.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#ifndef Utilitarianism_hpp
#define Utilitarianism_hpp

#include "Expecter.hpp"
#include "MoralTheory.hpp"
#include "Successor.hpp"
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
    std::string ToString() const override {

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
    WorthBase* clone() const override {
        return new ExpectedUtility(*this);
    }
    ExpectedUtility() {value=0;}
    explicit ExpectedUtility(double v) {value=v;}
    ExpectedUtility(const ExpectedUtility& other)  : WorthBase(other) {
        this->value = other.value;
    }
    ~ExpectedUtility() override = default;
    ExpectedUtility& operator=(WorthBase& w) override {
        if (const ExpectedUtility* eu = dynamic_cast<const ExpectedUtility*>(&w)) {
            this->value = eu->value;
        }
        return *this;
    }
    std::size_t hash() override {
        return std::hash<double>()(value);
    }
};



class Utilitarianism : public MoralTheory {
    std::unordered_map<Successor*, ExpectedUtility*> judgementMap;
    std::vector<double> heuristicList;

    static ExpectedUtility& quickCast(WorthBase& w) {
        return static_cast<ExpectedUtility&>(w);
    }
public:
    Utilitarianism(json &t, int id) : MoralTheory(id) {
        label = t["Name"];
        rank = t["Rank"];
        // Process heuristics
        for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->heuristicList.push_back(it.value());
        }
    }
    explicit Utilitarianism(int id_) : MoralTheory(id_) {
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
        ExpectedUtility* eu = new ExpectedUtility();
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
    int attack(QValue& qv1, QValue& qv2) override;
};




#endif /* Utilitarianism_hpp */
