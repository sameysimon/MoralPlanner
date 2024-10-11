//
// Created by Simon Kolker on 11/10/2024.
//

#ifndef ABSOLUTISM_HPP
#define ABSOLUTISM_HPP

#include "Expecter.hpp"
#include "MoralTheory.hpp"
#include "Successor.hpp"
#include <cmath>
#include <sstream>

class AbsoluteValue : public WorthBase {
public:
    bool value=false;
    // Use simple numeric operators
    int compare(WorthBase& wb) const override {
        AbsoluteValue* ab = static_cast<AbsoluteValue*>(&wb);
        if (value and !ab->value)
            return -1;
        if (!value and ab->value)
            return 1;
        return 0;
    }
    std::string ToString() const override {
        if (value) { return "T"; }
        return "F";
    }
    bool isEquivalent(WorthBase& w) const override {
        AbsoluteValue* ab = dynamic_cast<AbsoluteValue*>(&w);
        if (ab==nullptr)
            throw std::invalid_argument("Expected WorthBase to be of type AbsoluteWorth");
        return ab->value == value;
    }
    WorthBase* clone() const override {
        return new AbsoluteValue(*this);
    }
    AbsoluteValue() {value=false;}
    AbsoluteValue(const AbsoluteValue& other) {
        this->value = other.value;
    }
    ~AbsoluteValue() = default;
    AbsoluteValue& operator=(WorthBase& w) override {
        if (const AbsoluteValue* eu = dynamic_cast<const AbsoluteValue*>(&w)) {
            this->value = eu->value;
        }
        return *this;
    }
    std::size_t hash() override {
        return std::hash<double>()(value);
    }
};




class Absolutism : public MoralTheory {
    std::unordered_map<Successor*, AbsoluteValue*> judgementMap;
    std::vector<bool> heuristicList;
    AbsoluteValue& quickCast(WorthBase& w) {
        return static_cast<AbsoluteValue&>(w);
    }
public:
    Absolutism(int id_) : MoralTheory(id_) {
        judgementMap = std::unordered_map<Successor*, AbsoluteValue*>();
    }
    //
    // Getters
    //
    AbsoluteValue* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability=false) override {
        AbsoluteValue* ab;
        for (int i = 0; i < successors.size(); i++) {
            AbsoluteValue* j = judge(*successors[i]);
            ab = static_cast<AbsoluteValue*>(baselines[i]);// May be better way to do this?
            if (j->value or ab->value) {
                ab = new AbsoluteValue();
                ab->value = true;
                return ab;
            }
        }
        ab = new AbsoluteValue();
        ab->value = false;
        return ab;
    };
    WorthBase* newHeuristic(State& s) override {
        AbsoluteValue* eu = new AbsoluteValue();
        eu->value = heuristicList[s.id];
        return eu;
    };
    WorthBase* newWorth() override {
        return new AbsoluteValue();
    }

    //
    // Initialisation
    //
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        bool val = successorData;
        auto ab = new AbsoluteValue();
        ab->value = val;
        this->judgementMap.insert(std::make_pair(successor, ab));
    }
    void processHeuristics(nlohmann::json& heuristicData) override {
        for (auto it = heuristicData.begin(); it != heuristicData.end(); it++) {
            this->heuristicList.push_back(it.value());
        }
    }
    int attack(QValue& qv1, QValue& qv2) override;
};




#endif //ABSOLUTISM_HPP
