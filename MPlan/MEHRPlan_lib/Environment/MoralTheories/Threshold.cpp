//
// Created by Simon Kolker on 01/10/2024.
//

#include "Threshold.hpp"
#include "QValue.hpp"

int MEHRThreshold::attack(QValue& qv1, QValue& qv2) {
    auto val_1 = static_cast<ExpectedValue*>(qv1.expectations[considerationIdx]);
    auto val_2 = static_cast<ExpectedValue*>(qv2.expectations[considerationIdx]);

    if (val_1->value >= threshold and val_2->value < threshold) {
        return 1;
    }
    if (val_1->value < threshold and val_2->value >= threshold) {
        return -1;
    }
    return 0;
}
double MEHRThreshold::CriticalQuestionOne(int sourceSol, int targetSol, std::vector<std::vector<History*>> &histories) {
    throw std::runtime_error("MEHRThreshold: CriticalQuestionOne() not implemented");
}

int MEHRThreshold::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}