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
        // TODO *somehow* move this one layer up, so only sets of WorthBases can be compared.
        ExpectedUtility* eu = static_cast<ExpectedUtility*>(&w);
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
};



class Utilitarianism : public MoralTheory {
    // TODO: Change this to id system instead of addressing Successor by pointer which is cringe.
    std::unordered_map<Successor*, ExpectedUtility*> judgementMap;
    std::vector<ExpectedUtility> heuristics;
public:
    Utilitarianism() {
        judgementMap = std::unordered_map<Successor*, ExpectedUtility*>();
        heuristics = std::vector<ExpectedUtility>();
    }
    ExpectedUtility* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& expectations) override {
        double utility = 0;
        ExpectedUtility* ex;
        for (int i = 0; i < successors.size(); i++) {
            ExpectedUtility* j = judge(*successors[i]);
            ex = static_cast<ExpectedUtility*>(expectations[i]);// May be better way to do this?

            utility += successors[i]->probability * (j->value + ex->value);
        }

        ex = new ExpectedUtility();
        ex->value = utility;
        return ex;
    };
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        double val = successorData;
        auto u = new ExpectedUtility();
        u->value = val;
        this->judgementMap.insert(std::make_pair(successor, u));
    }
    ExpectedUtility* getHeuristic(State& state) {
        return &(this->heuristics[state.id]);
    };
    WorthBase* newExpectation() override {
        return new ExpectedUtility();
    }
    ExpectedUtility& quickCast(WorthBase& w) {
        return static_cast<ExpectedUtility&>(w);
    }
};




#endif /* Utilitarianism_hpp */
