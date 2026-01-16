//
// Created by Simon Kolker on 11/04/2025.
//


// Minimise probability of violating pode or probability of bad following good. Good must come first.
// Pass back prob of bad action
// Pass back bad consequences.
// Pass back

#pragma once
#include <format>

#include "MoralTheory.hpp"
using namespace std;

class PoDEWorth : public WorthBase {
public:
    double violationPr = 0.0;
    double goalPr = 0.0;
    double badPr = 0.0;

    // Use simple numeric operators
    int compare(WorthBase& wb) const override {
        auto other = static_cast<PoDEWorth*>(&wb);
       if (violationPr < other->violationPr) {
           return 1; // Try to minimise pode.
       }
        if (violationPr > other->violationPr) {
            return -1;
        }
        return 0;
    }


    [[nodiscard]] string ToString() const override {

    }
    bool isEquivalent(WorthBase& w) const override {
        auto pode_worth = dynamic_cast<PoDEWorth*>(&w);
        if (pode_worth==nullptr) {
            throw std::invalid_argument("Expected WorthBase to be of type Principle of Double Effect Worth");
            return false;
        }
        return
            (std::abs(goalPr - pode_worth->goalPr) < 1e-3)
            && (std::abs(badPr - pode_worth->badPr) < 1e-3)
            && (std::abs(violationPr - pode_worth->violationPr) < 1e-3);
    }

    [[nodiscard]] unique_ptr<WorthBase> clone() const override {
        return make_unique<PoDEWorth>(*this);
    }
    PoDEWorth() = default;
    PoDEWorth(double goalPr_, double badPr_, double violationPr_) :
        violationPr(violationPr_), badPr(badPr_), goalPr(goalPr_) {}

    PoDEWorth(const PoDEWorth& other)  : WorthBase(other) {
        this->goalPr = other.goalPr;
        this->violationPr = other.violationPr;
        this->badPr = other.badPr;
    }
    ~PoDEWorth() override = default;

    PoDEWorth& operator=(WorthBase& w) {
        if (auto pode_worth = dynamic_cast<const PoDEWorth*>(&w)) {
            this->violationPr = pode_worth->violationPr;
            this->goalPr = pode_worth->goalPr;
            this->badPr = pode_worth->badPr;
        }
        return *this;
    }
    std::size_t hash() override {
        // TODO: Make this hash the worth *properly*.
        auto h = std::hash<double>()(goalPr);
        h ^= std::hash<double>()(badPr);
        h ^= std::hash<double>()(violationPr);
        return h;
    }
};






class DoubleEffect : Consideration {
    std::unordered_map<Successor*, PoDEWorth*> judgementMap;
    //std::vector<PoDE> heuristicList;
public:
    bool sortHistories = false;

    DoubleEffect(json &t, size_t id) : Consideration(id) {
        label = t["Name"];
        /* Process heuristics
        for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->heuristicList.push_back(it.value());
        }*/
    }


    PoDEWorth* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability=false) override {
        double violationPr = 0;
        PoDEWorth* base;
        for (int i = 0; i < successors.size(); i++) {
            PoDEWorth* j = judge(*successors[i]);
            base = static_cast<PoDEWorth*>(baselines[i]);
            if (j->badPr == 1) {
                violationPr += j->violationPr;
            }
        }
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



