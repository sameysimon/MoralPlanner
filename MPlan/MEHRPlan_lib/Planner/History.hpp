//
// Created by Simon Kolker on 13/01/2026.
//


#pragma once
#include "QValue.hpp"
#include "Successor.hpp"


class HistoryPtrHash;
class HistoryPtrEqual;

class History {
public:
    QValue mWorth;
    mutable double probability;

    bool hasPath = false;
    // States visited in reverse order. path[0] is final state.
    unique_ptr<vector<size_t>> path = nullptr;

    explicit History(MDP& mdp, double _probability=1, bool usePath=false) : probability(_probability), hasPath(usePath) {
        mWorth = QValue(mdp);
        if (hasPath) {
            path = make_unique<vector<size_t>>();
        }
    }
    explicit History(QValue _worth, double _probability=1, bool usePath=false) : mWorth(std::move(_worth)), probability(_probability), hasPath(usePath) {
        if (hasPath) {
            path = make_unique<vector<size_t>>();
        }
    }
    History(const History& hist) {
        mWorth = hist.mWorth;
        probability = hist.probability;
        hasPath = hist.hasPath;
        if (hist.hasPath && hist.path!=nullptr) {
            path = make_unique<vector<size_t>>(hist.path->size());
            for (size_t i = 0; i < hist.path->size(); ++i) {
                (*path)[i] = hist.path->at(i);
            }
        }
    }
    History() {
        probability=1;
        hasPath=false;
    }
    History& operator=(const History& other) {
        if (this == &other) return *this;
        mWorth = other.mWorth;
        probability = other.probability;
        hasPath = other.hasPath;
        path = other.path ? std::make_unique<std::vector<size_t>>(*other.path) : nullptr;
        return *this;
    }

    bool operator==(const History& other) const {
        return mWorth == other.mWorth && (abs(probability - other.probability) < 0.001) && hasPath == other.hasPath;
    }
    bool isEquivalent(const History& other) const {
        double d = abs(probability - other.probability);
        return mWorth == other.mWorth && d < 0.000001;
    }
};

typedef vector<vector<unique_ptr<History>>> policy_hists;

class UHistoryPtrHash {
public:
    std::size_t operator()(const std::unique_ptr<History>& h) const noexcept {
        QValueHash qValHash;
        return qValHash(h->mWorth);
    }
};

class UHistoryPtrEqual {
public:
    bool operator()(const std::unique_ptr<History>& lhs, const std::unique_ptr<History>& rhs) const noexcept {
        return lhs->mWorth == rhs->mWorth;
    }
};

class HistoryPtrHash {
public:
    std::size_t operator()(const History* h) const noexcept {
        QValueHash qValHash;
        return qValHash(h->mWorth);
    };
};

class HistoryPtrEqual {
public:
    bool operator()(const History* lhs, const History* rhs) const noexcept {
        return lhs->mWorth == rhs->mWorth;
    }
};