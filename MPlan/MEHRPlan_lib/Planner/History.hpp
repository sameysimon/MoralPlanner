//
// Created by Simon Kolker on 13/01/2026.
//


#pragma once
#include "QValue.hpp"
#include "Successor.hpp"

class History {
public:
    QValue worth;
    mutable double probability;
    bool hasPath = false;
    unique_ptr<vector<Successor*>> path = nullptr;
    explicit History(QValue _worth, double _probability=1, bool usePath=false) : worth(std::move(_worth)), probability(_probability), hasPath(usePath) {
        if (hasPath) {
            path = make_unique<vector<Successor*>>();
        }
    }
    History(const History& hist) {
        worth = hist.worth;
        probability = hist.probability;
        hasPath = hist.hasPath;
        if (hist.hasPath && hist.path!=nullptr) {
            path = make_unique<vector<Successor*>>(hist.path->size());
            for (size_t i = 0; i < hist.path->size(); ++i) {
                (*path)[i] = hist.path->at(i);
            }
        }
    }
    History& operator=(const History& other) {
        if (this == &other) return *this;
        worth = other.worth;
        probability = other.probability;
        hasPath = other.hasPath;
        path = other.path ? std::make_unique<std::vector<Successor*>>(*other.path) : nullptr;
        return *this;
    }

    bool operator==(const History& other) const {
        return worth == other.worth && (abs(probability - other.probability) < 0.001) && hasPath == other.hasPath;
    }
    bool isEquivalent(const History& other) const {
        double d = abs(probability - other.probability);
        return worth == other.worth && d < 0.001;
    }
    void addToPath(Successor* successor) const {
        if (!hasPath || path == nullptr) {
            return;
        }
        path->push_back(successor);
    }

};
