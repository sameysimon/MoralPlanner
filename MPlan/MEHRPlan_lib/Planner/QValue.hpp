//
// Created by Simon Kolker on 18/09/2024.
//
#ifndef QVALUE_HPP
#define QVALUE_HPP

#include <utility>
#include "MoralTheory.hpp"



// Object for pointing to worth values in a solution/elsewhere (WorthBase*)
// Instantiated by calling vectorGather on a Solution. Holds
// Used to hold/compare/add state-time-action's worth/state's estimation easily.
class QValue {
public:
    std::vector<WorthBase*> expectations;
    QValue() {
        expectations = std::vector<WorthBase*>();
    }
    QValue(std::vector<WorthBase*> expectations_) {
        expectations = std::move(expectations_);
    }
    QValue(const QValue& other) {
        expectations = std::vector<WorthBase*>(other.expectations.size());
        for (int i = 0; i < other.expectations.size(); i++) {
            expectations[i] = other.expectations[i]->clone();
        }
    }

    bool greaterThan(QValue& qval);
    void addToExpectations(WorthBase* ev) {
        expectations.push_back(ev);
    };
    [[nodiscard]] std::string toString() const {
        std::string result = "";
        for (WorthBase* ev : expectations) {
            result += ev->ToString() + "; ";
        }
        return result;
    }
    [[nodiscard]] bool isEquivalent(const QValue& other) const {
        for (int i = 0; i < other.expectations.size(); i++) {
            if (!expectations[i]->isEquivalent(*other.expectations[i])) {
                return false;
            }
        }
        return true;
    }
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
        // CLion says hash_combine is unreachable??
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);  // A common hash combine technique. Check this out.
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


#endif //QVALUE_HPP
