//
// Created by Simon Kolker on 11/10/2024.
//

#ifndef ABSOLUTISM_HPP
#define ABSOLUTISM_HPP

#include "MoralTheory.hpp"
#include "Successor.hpp"
#include "State.hpp"
#include "HistoryHandler.hpp"
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
    [[nodiscard]] unique_ptr<WorthBase> clone() const override {
        return make_unique<AbsoluteValue>(*this);
    }
    AbsoluteValue() {value=false;}
    AbsoluteValue(const AbsoluteValue& other) {
        this->value = other.value;
    }
    ~AbsoluteValue() override = default;
    AbsoluteValue& operator=(WorthBase& w) {
        if (const AbsoluteValue* eu = dynamic_cast<const AbsoluteValue*>(&w)) {
            this->value = eu->value;
        }
        return *this;
    }
    std::size_t hash() override {
        return std::hash<double>()(value);
    }
};




class Absolutism : public Consideration {
    std::unordered_map<Successor*, AbsoluteValue*> judgementMap;
    std::vector<bool> heuristicList;
    static AbsoluteValue& quickCast(WorthBase& w) {
        return static_cast<AbsoluteValue&>(w);
    }
public:
    explicit Absolutism(size_t id_) : Consideration(id_) {
        judgementMap = std::unordered_map<Successor*, AbsoluteValue*>();
    }
    Absolutism(json &t, size_t id_) : Consideration(id_) {
        label = t["Name"];
        for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->heuristicList.push_back(it.value());
        }
    }
    //
    // Getters
    //
    AbsoluteValue* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    unique_ptr<WorthBase> gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        AbsoluteValue* ab;
        for (int i = 0; i < successors.size(); i++) {
            AbsoluteValue* j = judge(*successors[i]);
            ab = static_cast<AbsoluteValue*>(baselines[i]);// May be better way to do this?
            if (j->value or ab->value) {
                auto r = make_unique<AbsoluteValue>();
                r->value = true;
                return r;
            }
        }
        auto r = make_unique<AbsoluteValue>();
        r->value = false;
        return r;
    };
    unique_ptr<WorthBase> newHeuristic(State& s) override {
        auto eu = make_unique<AbsoluteValue>();
        eu->value = heuristicList[s.id];
        return eu;
    };
    WorthBase* newWorth() override {
        return new AbsoluteValue();
    }

    std::unique_ptr<WorthBase> UniqueWorth() override {
        return std::make_unique<AbsoluteValue>();
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
};

class MEHRAbsolutism : public MEHRTheory {
    SortHistories *pSortedHistories;
public:
    size_t mConsiderationIdx;

    MEHRAbsolutism(size_t rank_, size_t theoryIdx_, std::string name_) : MEHRTheory(rank_, theoryIdx_, name_) {
        pSortedHistories = new SortHistories(*this);
    }
    int attack(QValue& qv1, QValue& qv2) override;
    Attack CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>>& histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->InitMEHR(histories);
    }
    void AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->AddPolicyHistories(histories);
    }
    void AddConsideration(Consideration& con) override {
        mConsiderationIdx = con.id;
    }
};




#endif //ABSOLUTISM_HPP
