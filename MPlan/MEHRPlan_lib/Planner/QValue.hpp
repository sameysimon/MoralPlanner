//
// Created by Simon Kolker on 18/09/2024.
//

#include "MoralTheory.hpp"

#ifndef QVALUE_HPP
#define QVALUE_HPP

// Object for pointing to worth values in a solution/elsewhere (WorthBase*)
// Instantiated by calling vectorGather on a Solution. Holds
// Used to hold/compare/add state-time-action's worth/state's estimation easily.
class QValue {
public:
    std::vector<WorthBase*> expectations;
    QValue() {
        expectations = std::vector<WorthBase*>();
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
    std::string toString() {
        std::string result = "";
        for (WorthBase* ev : expectations) {
            result += ev->ToString() + "; ";
        }
        return result;
    }
};

class QValueHash {
public:
    static void hash_combine(std::size_t& seed, const std::size_t& hash) {
        // CLion says hash_combine is unreachable??
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);  // A common hash combine technique. Check this out.
    }
    std::size_t operator()(const QValue& qVal) const noexcept {
        std::size_t seed = 0;
        for (WorthBase* wb : qVal.expectations) {
            std::size_t wbHashed = wb->hash();
            hash_combine(seed, wbHashed);
        }
        return seed;
    }
};

class QValueEqual {
public:
    bool operator()(const QValue& lhs, const QValue& rhs) const noexcept {
        if (lhs.expectations.size() != rhs.expectations.size()) {
            return false;
        }
        for (int i = 0; i < lhs.expectations.size(); i++) {
            if (not lhs.expectations[i]->isEquivalent(*rhs.expectations[i])) {
                return false;
            }
        }
        return true;
    }
};


#endif //QVALUE_HPP
