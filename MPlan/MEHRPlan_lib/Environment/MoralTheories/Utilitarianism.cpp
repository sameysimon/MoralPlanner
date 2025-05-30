
#include "Utilitarianism.hpp"
#include "QValue.hpp"

int MEHRUtilitarianism::attack(QValue& qv1, QValue& qv2) {
    return qv1.expectations[considerationIdx]->compare(*qv2.expectations[considerationIdx]);
}
double MEHRUtilitarianism::CriticalQuestionOne(int sourceSol, int targetSol, std::vector<std::vector<History*>>& histories) {
    return pSortedHistories->CriticalQuestionOne(sourceSol, targetSol, histories);
}


int MEHRUtilitarianism::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}
