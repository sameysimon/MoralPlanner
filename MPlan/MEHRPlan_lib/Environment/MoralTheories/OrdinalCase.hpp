//
// Created by Simon Kolker on 20/12/2025.
//

#pragma once
#include "MoralTheory.hpp"
#include "HistoryHandler.hpp"
#include "State.hpp"
#include <sstream>

class OrdinalWorth : public WorthBase {
public:
    // Higher is better
    int mValue = 0;

    // Use simple numeric operators
    int compare(WorthBase& wb) const override {
        auto oth = static_cast<OrdinalWorth*>(&wb);
        if (mValue < oth->mValue) {
            return -1;
        }
        if (mValue > oth->mValue) {
            return 1;
        }
        return 0;
    }
    [[nodiscard]] std::string ToString() const override {
        return std::format("Rank:{}", mValue);
    }
    bool isEquivalent(WorthBase& w) const override {
        auto oth = static_cast<OrdinalWorth*>(&w);
        return mValue==oth->mValue;
    }
    [[nodiscard]] unique_ptr<WorthBase> clone() const override {
        return make_unique<OrdinalWorth>(*this);
    }
    OrdinalWorth() = default;
    ~OrdinalWorth() override = default;
    OrdinalWorth(const OrdinalWorth& other) {
        this->mValue = other.mValue;
    }
    OrdinalWorth& operator=(WorthBase& w) {
        if (auto* oth = dynamic_cast<const OrdinalWorth*>(&w)) {
            this->mValue = oth->mValue;
        }
        return *this;
    }
    std::size_t hash() override {
        return std::hash<int>()(mValue);
    }
};

class Ordinal : public Consideration {
    std::unordered_map<Successor*, OrdinalWorth*> mJudgementMap;
    std::vector<double> mHeuristicList;
    // When mOptimalityType=0, optimise for worst case,
    // When mOptimalityType=1, optimise for best case,
    // When mOptimalityType=2, optimise for average case,
    int mOptimalityType = 0;

    static OrdinalWorth& quickCast(WorthBase& w) {
        return static_cast<OrdinalWorth&>(w);
    }
public:
    explicit Ordinal(size_t id) : Consideration(id) {
        mJudgementMap = std::unordered_map<Successor*, OrdinalWorth*>();
    }
    Ordinal(json& t, size_t id) : Consideration(id) {
        label = t["Name"];
        mOptimalityType = t["Optimality_type"];
        mJudgementMap = std::unordered_map<Successor*, OrdinalWorth*>();
    }
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        int val = successorData;
        auto u = new OrdinalWorth();
        u->mValue = val;
        this->mJudgementMap.insert(std::make_pair(successor, u));
    }
    //
    // Getters
    //
    OrdinalWorth* judge(Successor& successor) {
        return mJudgementMap[&successor];
    }
    unique_ptr<WorthBase> gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        OrdinalWorth* optimal = judge(*successors[0]);
        OrdinalWorth* j;

        for (int i = 0; i < successors.size(); i++) {
            j = judge(*successors[i]);
            if (mOptimalityType==0 && j->compare(*optimal) == -1) {
                optimal = j;
            }
            if (mOptimalityType==1 && j->compare(*optimal) == 1) {
                optimal = j;
            }
            j = static_cast<OrdinalWorth*>(baselines[i]);
            if (mOptimalityType==0 && j->compare(*optimal) == 1) {
                optimal = j;
            }
            if (mOptimalityType==1 && j->compare(*optimal) == -1) {
                optimal = j;
            }
        }
        return make_unique<OrdinalWorth>(*optimal);
    }

    std::unique_ptr<WorthBase> UniqueWorth() override {
        return std::make_unique<OrdinalWorth>();
    }
    std::unique_ptr<WorthBase> newHeuristic(State& s) override {
        return std::make_unique<OrdinalWorth>();
    };
    WorthBase* newWorth() override {
        return new OrdinalWorth();
    }
};

class MEHROrdinal : public MEHRTheory {
    size_t considerationIdx=0;
    SortHistories *pSortedHistories;
public:
    MEHROrdinal(size_t rank_, size_t theory_id, std::string &name_) : MEHRTheory(rank_, theory_id, name_) {
        pSortedHistories = new SortHistories(*this);
    }
    /*~MEHRUtilitarianism() override {
     *Not sure if I need this to get rid of pSortedHistories, if it will deal with base class stuff
        delete pSortedHistories;
    }*/
    int attack(QValue& qv1, QValue& qv2) override;
    Attack CriticalQuestionOne(Attack& att, std::vector<std::vector<History*>>& histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->InitMEHR(histories);

    }
    void AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->AddPolicyHistories(histories);
    }
    SortHistories* getSortedHistories() {
        return pSortedHistories;
    }
    void AddConsideration(Consideration& con) override {
        considerationIdx = con.id;
    };
};