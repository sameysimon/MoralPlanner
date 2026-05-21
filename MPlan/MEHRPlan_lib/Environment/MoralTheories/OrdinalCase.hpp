//
// Created by Simon Kolker on 20/12/2025.
//

#pragma once
#include "MoralTheory.hpp"
#include "HistoryHandler.hpp"
#include "State.hpp"
#include <sstream>
#include <format>

class OrdinalWorth : public WorthBase {
public:
    // Higher is better
    int mValue = 0;
    OrdinalWorth(size_t rank) {
        mValue = rank;
    }

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
        return std::format("R={}", mValue);
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
    //std::unordered_map<Successor*, unique_ptr<OrdinalWorth>, SuccessorHash, SuccessorEqual> mJudgementMap;
    vector<vector<vector<pair<size_t, unique_ptr<OrdinalWorth>>>>> judgements;

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
        //mJudgementMap = std::unordered_map<Successor*, unique_ptr<OrdinalWorth>, SuccessorHash, SuccessorEqual>();
    }
    Ordinal(json& t, size_t id, size_t state_space_reserve) : Consideration(id) {
        label = t["Name"];
        mOptimalityType = t["Optimality_type"];
        //mJudgementMap = std::unordered_map<Successor*, unique_ptr<OrdinalWorth>, SuccessorHash, SuccessorEqual>();
        judgements.resize(state_space_reserve);
        for (auto i = 0; i < state_space_reserve; ++i) {
            judgements[i].resize(state_space_reserve);
        }

    }
    void processSuccessor(Successor* successor, nlohmann::json &successorData) override {
        int val = successorData;
        auto ow = make_unique<OrdinalWorth>(val);
        judgements[successor->source][successor->target].push_back( pair(successor->action_idx, std::move(ow)));
    }


    //
    // Getters
    //
    WorthBase* judge(Successor& successor) override {
        for (auto &ow : judgements[successor.source][successor.target]) {
            if (ow.first == successor.action_idx) {
                return ow.second.get();
            }
        }
        throw format("Cannot find moral worth of successor {} -> {} by action ID {}", successor.source, successor.target, successor.action_idx);
    }
    unique_ptr<WorthBase> gather(const std::vector<WorthBase*>& worth, const std::vector<double>& probs, const std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        OrdinalWorth* optimal = static_cast<OrdinalWorth*>(worth[0]);
        OrdinalWorth* j;

        for (int i = 0; i < worth.size(); i++) {
            j = static_cast<OrdinalWorth*>(worth[i]);
            if (mOptimalityType==0 && j->compare(*optimal) == -1) {
                // worst mode, pick the worst one
                optimal = j;
            }
            if (mOptimalityType==1 && j->compare(*optimal) == 1) {
                // best mode, pick the best one
                optimal = j;
            }
            j = static_cast<OrdinalWorth*>(baselines[i]);
            if (mOptimalityType==0 && j->compare(*optimal) == -1) {
                // worst mode, pick the worst one
                optimal = j;
            }
            if (mOptimalityType==1 && j->compare(*optimal) == 1) {
                // best mode, pick the best one
                optimal = j;
            }
        }
        return make_unique<OrdinalWorth>(optimal->mValue);
    }
    std::vector<double> normalise(std::vector<WorthBase*> &worth_vec) override {
        std::vector<double> r;
        auto max_worth = std::max_element(worth_vec.begin(), worth_vec.end(), [](auto a, auto b) {
            return a->compare(*b) == 1;
        });
        auto min_worth = std::min_element(worth_vec.begin(), worth_vec.end(), [&](auto a, auto b) {
            return a->compare(*b) == 1;
        });
        auto max_util = quickCast(**max_worth.base()).mValue;
        auto min_util = quickCast(**min_worth.base()).mValue;
        r.reserve(worth_vec.size());
        for (auto wb : worth_vec) {
            r.push_back( (quickCast(*wb).mValue - min_util) / (max_util - min_util + 0.0) );
        }
        return r;
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
    Attack CriticalQuestionOne(Attack& att, policy_hists& histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<std::unique_ptr<History>>> &histories) override {
        pSortedHistories->InitMEHR(histories);

    }
    void AddPoliciesForMEHR(policy_hists &histories) override {
        pSortedHistories->AddPolicyHistories(histories);
    }
    SortHistories* getSortedHistories() {
        return pSortedHistories;
    }
    void AddConsideration(Consideration& con) override {
        considerationIdx = con.id;
    };
};