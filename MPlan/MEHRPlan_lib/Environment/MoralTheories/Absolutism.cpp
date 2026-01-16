//
// Created by Simon Kolker on 11/10/2024.
//

#include "Absolutism.hpp"
#include "QValue.hpp"
#include "ExtractHistories.hpp"

int MEHRAbsolutism::attack(QValue& qv1, QValue& qv2) {
    return qv1.expectations[mConsiderationIdx]->compare(*qv2.expectations[mConsiderationIdx]);
}

int MEHRAbsolutism::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return qv1.expectations[mConsiderationIdx]->compare(*qv2.expectations[mConsiderationIdx]);
}

Attack MEHRAbsolutism::CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>> &histories) {
    a.theoryIdx = mId;
    return pSortedHistories->CriticalQuestionOne(a, histories);

}