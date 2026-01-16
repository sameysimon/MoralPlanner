//
// Created by Simon Kolker on 20/12/2025.
//

#include "OrdinalCase.hpp"
#include "QValue.hpp"

int MEHROrdinal::attack(QValue& qv1, QValue& qv2) {
    return qv1.expectations[considerationIdx]->compare(*qv2.expectations[considerationIdx]);
}
Attack MEHROrdinal::CriticalQuestionOne(Attack& att, std::vector<std::vector<History*>>& histories) {
    att.theoryIdx = mId;
    return pSortedHistories->CriticalQuestionOne(att, histories);
}
int MEHROrdinal::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}
