//
// Created by Simon Kolker on 01/10/2024.
//

#include "Threshold.hpp"
#include "QValue.hpp"

int MEHRThreshold::attack(QValue& qv1, QValue& qv2) {
    auto val_1 = static_cast<FactoredUtility*>(qv1.expectations[considerationIdx].get());
    auto val_2 = static_cast<FactoredUtility*>(qv2.expectations[considerationIdx].get());
    return 0;
}
Attack MEHRThreshold::CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>> &histories) {
    throw std::runtime_error("MEHRThreshold: CriticalQuestionOne() not implemented");
}

int MEHRThreshold::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}