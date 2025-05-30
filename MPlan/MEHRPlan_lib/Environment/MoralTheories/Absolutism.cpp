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

double MEHRAbsolutism::CriticalQuestionOne(int sourceSol, int targetSol, std::vector<std::vector<History*>> &histories) {
    return pSortedHistories->CriticalQuestionOne(sourceSol, targetSol, histories);

}