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
#include <sstream>
#include <iomanip>

static std::string doubleToString(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(15) << v; // Convert with maximum precision
    std::string str = oss.str();

    // Strip trailing zeros
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    // If there's a decimal point left at the end, remove it
    if (str.back() == '.') {
        str.pop_back();
    }
    return str;
}

class ExpectedUtility : public WorthBase {
public:
    double value=0;
    // Use simple numeric operators
    int compare(WorthBase& wb) const override {
        ExpectedUtility* eu = static_cast<ExpectedUtility*>(&wb);
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
        return (abs(value - eu->value) < 1e-3);
    }
    WorthBase* clone() const override {
        return new ExpectedUtility(*this);
    }
    ExpectedUtility() {value=0;}
    ExpectedUtility(const ExpectedUtility& other) {
        this->value = other.value;
    }
    ~ExpectedUtility() = default;
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
    std::vector<int> heuristicList;

    ExpectedUtility& quickCast(WorthBase& w) {
        return static_cast<ExpectedUtility&>(w);
    }
public:
    Utilitarianism() {
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
            ex = static_cast<ExpectedUtility*>(baselines[i]);// May be better way to do this?
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
    void processHeuristics(nlohmann::json& heuristicData) override {
        for (auto it = heuristicData.begin(); it != heuristicData.end(); it++) {
            this->heuristicList.push_back(it.value());
        }
    }
};




#endif /* Utilitarianism_hpp */
