//
// Created by Simon Kolker on 01/10/2024.
//

#ifndef THRESHOLD_HPP
#define THRESHOLD_HPP

#include "MoralTheory.hpp"
#include "Successor.hpp"
#include "State.hpp"
#include <cmath>
#include <sstream>

#include "Utilitarianism.hpp"

// The Morally Relevant Information
class FactoredUtility : public WorthBase {
public:
    double bad_probability = 0;
    double lastValue = 0;
    std::vector<double> values;
    std::vector<double> values_probability;
    // Use simple numeric operators
    int compare(WorthBase& wb) const override {
        auto oth = static_cast<FactoredUtility*>(&wb);
        if (bad_probability < oth->bad_probability) {
            return 1;
        }
        if (bad_probability > oth->bad_probability) {
            return -1;
        }
        return 0;
    }
    [[nodiscard]] std::string ToString() const override {
        std::ostringstream os;
        for (size_t i=0; i < values.size(); i++) {
            os << values[i] << " at " << values_probability[i];
        }
        os << "Bad at " << bad_probability;
        return os.str();
    }
    bool isEquivalent(WorthBase& w) const override {
        FactoredUtility* oth = dynamic_cast<FactoredUtility*>(&w);
        if (oth==nullptr) {
            throw std::invalid_argument("Expected WorthBase to be of type FactoredUtility");
            return false;
        }
        return (std::abs(bad_probability - oth->bad_probability) < 1e-3);
    }
    [[nodiscard]] unique_ptr<WorthBase> clone() const override {
        return make_unique<FactoredUtility>(*this);
    }
    FactoredUtility() = default;
    ~FactoredUtility() = default;
    FactoredUtility(const FactoredUtility& other) {
        this->values = other.values;
        this->values_probability = other.values_probability;
        this->bad_probability = other.bad_probability;
    }
    FactoredUtility& operator=(WorthBase& w) {
        if (auto* oth = dynamic_cast<const FactoredUtility*>(&w)) {
            this->values = oth->values;
            this->values_probability = oth->values_probability;
            this->bad_probability = oth->bad_probability;
        }
        return *this;
    }
    std::size_t hash() override {
        auto h = std::hash<double>()(bad_probability);
        for (size_t i=0; i < values.size(); i++) {
            h ^= std::hash<int>()(values[i]);
            h ^= std::hash<double>()(values_probability[i]);
        }
        return h;
    }
};


// The Moral Consideration
class Threshold : public Consideration {
    std::unordered_map<Successor*, FactoredUtility*> judgementMap;
    std::vector<double> heuristicList;
    double threshold = 0;

    static FactoredUtility& quickCast(WorthBase& w) {
        return static_cast<FactoredUtility&>(w);
    }
public:
    Threshold(size_t id) : Consideration(id) {
        judgementMap = std::unordered_map<Successor*, FactoredUtility*>();
    }
    Threshold(json& t, size_t id) : Consideration(id) {
        label = t["Name"];
        threshold = t["Threshold"];
        judgementMap = std::unordered_map<Successor*, FactoredUtility*>();
        /*for (auto it = t["Heuristic"].begin(); it != t["Heuristic"].end(); it++) {
            this->heuristicList.push_back(it.value());
        }*/
    }

    //
    // Getters
    //
    FactoredUtility* judge(Successor& successor) {
        return judgementMap[&successor];
    }
    unique_ptr<WorthBase> gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability) override {
        auto agg = make_unique<FactoredUtility>();
        return agg;
        /*
        FactoredUtility* base;
        for (int i = 0; i < successors.size(); i++) {
            FactoredUtility* j = judge(*successors[i]);
            double scrValue = j->lastValue;
            double scrPr = successors[i]->probability;
            base = static_cast<FactoredUtility*>(baselines[i]);
            for (int v_idx=0; v_idx < base->values.size(); v_idx++) {
                if (base->values[v_idx] + scrValue < threshold) {
                    agg->bad_probability = base->bad_probability + scrPr;
                    continue;
                }
                agg->values.push_back(base->values[v_idx] + scrValue);
                agg->values_probability.push_back(base->values_probability[v_idx] * scrPr);
            }


            double newVal = j->value + ex->value;
            if (not ignoreProbability) {
                newVal *= successors[i]->probability;
            }
            utility+=newVal;
        }

        ex = new FactoredUtility();
        ex->value = utility;
        */
    }
    unique_ptr<WorthBase> newHeuristic(State& s) override {
        auto eu = make_unique<FactoredUtility>();
        //eu->value = heuristicList[s.id];
        return eu;
    };
    WorthBase* newWorth() override {
        return new FactoredUtility();
    }
    //
    // Initialisation
    //
    void processSuccessor(Successor* successor, nlohmann::json successorData) override {
        double val = successorData;
        auto u = new FactoredUtility();
        //u->value = val;
        this->judgementMap.insert(std::make_pair(successor, u));
    }
};

class MEHRThreshold : public MEHRTheory {
    double threshold;
    SortHistories *pSortedHistories;
    size_t considerationIdx;
public:
    explicit MEHRThreshold(size_t rank_, size_t theoryID, double threshold, std::string &name_) : MEHRTheory(rank_, theoryID, name_), threshold(threshold) {
        pSortedHistories = new SortHistories(*this);
    }
    int attack(QValue& qv1, QValue& qv2) override;
    Attack CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>> &histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->InitMEHR(histories);
    }
    void AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) override {
        pSortedHistories->AddPolicyHistories(histories);
    }
    void AddConsideration(Consideration& con) override {
        considerationIdx = con.id;
    }

};



#endif //THRESHOLD_HPP
