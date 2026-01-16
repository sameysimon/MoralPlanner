//
//  MoralTheory.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#pragma once
#include <sstream>
#include <iomanip>
#include <string>
#include <nlohmann/json.hpp>
#include "Successor.hpp"


struct Attack {
    double p = 0;
    size_t sourcePolicyIdx;
    size_t targetPolicyIdx;
    size_t theoryIdx;
    // Stores {i,j} if source history i attacks target history j.
    std::vector<std::pair<size_t,size_t>> HistoryEdges;
    Attack(size_t sourcePolicyIdx, size_t targetPolicyIdx, size_t theoryIdx) : sourcePolicyIdx(sourcePolicyIdx), targetPolicyIdx(targetPolicyIdx), theoryIdx(theoryIdx) {};
    void addEdge(size_t sourceHistory, size_t targetHistory, double targetProbability) {
        HistoryEdges.emplace_back(sourceHistory, targetHistory);
        p += targetProbability;;
    }
    void addEdge(size_t sourceHistory, size_t targetHistory) {
        HistoryEdges.emplace_back(sourceHistory, targetHistory);
    }
    [[nodiscard]] bool isUndefined() const {
        return HistoryEdges.empty();
    }

};

class History;
using json = nlohmann::json;

class Expecter;
class QValue;
class State;

class WorthBase {
public:
    [[nodiscard]] virtual std::string ToString() const = 0;
    virtual int compare(WorthBase& wb) const = 0;
    virtual bool isEquivalent(WorthBase& w) const = 0;
    [[nodiscard]] virtual std::unique_ptr<WorthBase> clone() const = 0;
    WorthBase& operator=(WorthBase const& w) = default;
    virtual ~WorthBase() = default;
    virtual std::size_t hash() = 0;
};

class Consideration {
public:
    std::string label;
    size_t id = 0;
    bool sortHistories = true;
    Consideration() = default;
    explicit Consideration(size_t id_) : id(id_) {}
    virtual ~Consideration() = default;

    virtual std::unique_ptr<WorthBase> gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability) = 0;
    // Setup
    virtual WorthBase* newWorth() = 0;
    virtual std::unique_ptr<WorthBase> UniqueWorth() = 0;
    virtual std::unique_ptr<WorthBase> newHeuristic(State& s) = 0;
    virtual void processSuccessor(Successor* successor, nlohmann::json successorData) = 0;
    virtual void addComponent(Consideration* m) {};

    // Functional
    Expecter* makeExpecter(int size, int horizon);
    Expecter* makeHeuristicExpecter(std::vector<State*>& states, int horizon);
};

static std::string doubleToString(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << v;
    std::string str = oss.str();
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    if (str.back() == '.') {
        str.pop_back();
    }
    return str;
}

class MEHRTheory {
public:
    size_t mRank = 0;
    std::string mName;
    size_t mId = 0;
    explicit MEHRTheory(size_t rank_, size_t id_, std::string &name_) : mRank(rank_), mId(id_), mName(name_) {}
    virtual ~MEHRTheory() = default;

    // Finds the sum probability of Policy targetSol's attacked arguments from sourceSol, according to QValues in histories.
    // Handles removal of duplicate attacks on arguments.
    // Assumes sourceSol attacks targetSol by CQ2 -- expectations not used.
    // Method depends on Moral Theory. Some variant of comparing histories though.
    virtual Attack CriticalQuestionOne(Attack& att, std::vector<std::vector<History*>> &histories) = 0;

    virtual int CriticalQuestionTwo(QValue& qv1, QValue& qv2) = 0;

    virtual int attack(QValue& qv1, QValue& qv2) = 0;
    virtual void InitMEHR(std::vector<std::vector<History*>> &histories) = 0;
    virtual void AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) = 0;
    virtual void AddConsideration(Consideration &con) = 0;
};



