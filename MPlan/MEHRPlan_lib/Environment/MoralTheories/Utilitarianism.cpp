
#include "Utilitarianism.hpp"
#include "QValue.hpp"

int MEHRUtilitarianism::attack(QValue& qv1, QValue& qv2) {
    return qv1.expectations[considerationIdx]->compare(*qv2.expectations[considerationIdx]);
}
Attack MEHRUtilitarianism::CriticalQuestionOne(size_t sourceSol, size_t targetSol, std::vector<std::vector<History*>>& histories) {
    return pSortedHistories->CriticalQuestionOne(sourceSol, targetSol, histories, mId);
}


int MEHRUtilitarianism::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}
