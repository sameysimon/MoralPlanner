//
// Created by Simon Kolker on 18/09/2024.
//
#pragma once
#include <utility>

#include "MDP.hpp"
#include "MoralTheory.hpp"


using namespace std;

// Object for pointing to worth values in a solution/elsewhere (WorthBase*)
// Instantiated by calling vectorGather on a Solution. Holds
// Used to hold/compare/add state-time-action's worth/state's estimation easily.
class QValue {
public:
    vector<unique_ptr<WorthBase>> expectations;
    QValue() {
        expectations = vector<unique_ptr<WorthBase>>();
    }
    explicit QValue(size_t reserve) {
        expectations = vector<unique_ptr<WorthBase>>(reserve);
    }
    explicit QValue(MDP& mdp) {
        expectations = std::vector<unique_ptr<WorthBase>>(mdp.considerations.size());
        for (unsigned int i = 0; i < mdp.considerations.size(); i++) {
            expectations[i] = mdp.considerations[i]->UniqueWorth();
        }

    }
    explicit QValue(std::vector<unique_ptr<WorthBase>> expectations_) {
        expectations = std::move(expectations_);
    }
    QValue(const QValue& other) {
        expectations.resize(other.expectations.size());
        for (size_t i = 0; i < other.expectations.size(); ++i) {
            expectations[i] = other.expectations[i]
                ? other.expectations[i]->clone()
                : nullptr;
        }
    }
    QValue& operator=(const QValue& other) {
        if (this == &other) return *this;
        expectations.resize(other.expectations.size());
        for (size_t i = 0; i < other.expectations.size(); ++i) {
            expectations[i] = other.expectations[i]
                ? other.expectations[i]->clone()
                : nullptr;
        }
        return *this;
    }
    QValue(QValue&&) noexcept = default;
    QValue& operator=(QValue&&) noexcept = default;


    bool operator==(const QValue& other) const {
        if (expectations.size() != other.expectations.size()) {
            return false;
        }
        for (int i = 0; i < expectations.size(); i++) {
            if (not expectations[i]->isEquivalent(*other.expectations[i])) {
                return false;
            }
        }
        return true;
    }

    static void hash_combine(std::size_t& seed, const std::size_t& hash) {
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);  // A common hash combine technique. Check this out.
    }
    [[nodiscard]] bool isEquivalent(const QValue& other) const {
        for (int i = 0; i < other.expectations.size(); i++) {
            if (!expectations[i]->isEquivalent(*other.expectations[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::string toString() const {
        std::string result;
        for (auto &ev : expectations) {
            result += ev->ToString() + "; ";
        }
        return result;
    }
    [[nodiscard]] std::vector<std::string> toStringVector() const {
        std::vector<std::string> result;
        result.reserve(expectations.size());
        for (auto &ev : expectations) {
            result.push_back(ev->ToString());
        }
        return result;
    }
};


class QValueHash {
public:
    std::size_t operator()(const QValue& qVal) const noexcept;
};

class QValueEqual {
public:
    bool operator()(const QValue& lhs, const QValue& rhs) const noexcept {
        return lhs == rhs;
    }
};

