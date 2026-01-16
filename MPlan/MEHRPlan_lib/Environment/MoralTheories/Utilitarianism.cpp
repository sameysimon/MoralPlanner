
#include "Utilitarianism.hpp"
#include "QValue.hpp"

int MEHRUtilitarianism::attack(QValue& qv1, QValue& qv2) {
    return qv1.expectations[considerationIdx]->compare(*qv2.expectations[considerationIdx]);
}
Attack MEHRUtilitarianism::CriticalQuestionOne(Attack& att, std::vector<std::vector<History*>>& histories) {
    att.theoryIdx = mId;
    return pSortedHistories->CriticalQuestionOne(att, histories);
}


int MEHRUtilitarianism::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}
