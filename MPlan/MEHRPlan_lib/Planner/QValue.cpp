//
// Created by Simon Kolker on 18/09/2024.
//

#include "QValue.hpp"
#include "MoralTheory.hpp"
namespace std {
template <>
struct hash<QValue> {
    std::size_t operator()(const QValue& qval) const {
        std::size_t seed = 0;
        for (auto &wb : qval.expectations) {
            std::size_t wbHashed = wb->hash();
            QValue::hash_combine(seed, wbHashed);

        }
        return seed;
    }
};
}
std::size_t QValueHash::operator()(const QValue& qVal) const noexcept {
    return std::hash<QValue>()(qVal);  // Use the std::hash specialization
}