//
// Created by Simon Kolker on 11/04/2025.
//




#pragma once
#include "MoralTheory.hpp"
using namespace std;
class PoDEWorth : public WorthBase {
public:
    // Hard limit of 5 consequences at the moment, may change in the future...
    array<ushort, 5> consequences;
    bool actionGood = true;
    double moralGoalProbability = 1.0;
    double utility = 1.0;

    // Use simple numeric operators
    int compare(WorthBase& wb) const override { }

    [[nodiscard]] string ToString() const override {
        string out = actionGood ? "Good Action " : "Bad Action ";
        out += format("hits moral goal at P={}, uility={} w/ consequences", moralGoalProbability, utility);
        for (auto i : consequences) { out += format("{}, ", i); }
        out.erase(out.size() - 2);
        return out;
    }
    bool isEquivalent(WorthBase& w) const override {
        auto pode_worth = dynamic_cast<PoDEWorth*>(&w);
        if (pode_worth==nullptr) {
            throw std::invalid_argument("Expected WorthBase to be of type Principle of Double Effect Worth");
            return false;
        }
        return consequences==pode_worth->consequences
            && actionGood==pode_worth->actionGood
            && (std::abs(moralGoalProbability - pode_worth->moralGoalProbability) < 1e-3)
            && (std::abs(utility - pode_worth->utility) < 1e-3);
    }

    [[nodiscard]] WorthBase* clone() const override {
        return new PoDEWorth(*this);
    }
    PoDEWorth() = default;
    PoDEWorth(array<ushort, 5> cons_, bool actionGood_, double moralGoalProb_, double utility_) :
        consequences(cons_), actionGood(actionGood_), moralGoalProbability(moralGoalProb_), utility(utility_) {}

    PoDEWorth(const PoDEWorth& other)  : WorthBase(other) {
        this->consequences = other.consequences;
        this->actionGood = other.actionGood;
        this->moralGoalProbability = other.moralGoalProbability;
        this->utility = other.utility;
    }
    ~PoDEWorth() override = default;

    PoDEWorth& operator=(WorthBase& w) {
        if (const PoDEWorth* pode_worth = dynamic_cast<const PoDEWorth*>(&w)) {
            this->consequences = pode_worth->consequences;
            this->actionGood = pode_worth->actionGood;
            this->moralGoalProbability = pode_worth->moralGoalProbability;
            this->utility = pode_worth->utility;
        }
        return *this;
    }
    std::size_t hash() override {
        // TODO: Make this hash the worth *properly*.
        return std::hash<double>()(utility);
    }
};






class DoubleEffect : Consideration {
    std::unordered_map<Successor*, PoDEWorth*> judgementMap;
    //std::vector<double> heuristicList;
public:
    bool sortHistories = false;
    PoDEWorth* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability=false) override {
    }
    WorthBase* newWorth() override {
        return new PoDEWorth();
    }

    //
    // Initialisation
    //
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        bool actionGood = successorData["action"];
        double utility = successorData["utils"];
        bool moralGoal = successorData["goal"];
        array<ushort, 5> cons = successorData["cons"].get<array<ushort, 5>>();

        auto u = new PoDEWorth(cons, actionGood, moralGoal ? 1 : 0, utility);
        this->judgementMap.insert(std::make_pair(successor, u));
    }
};



