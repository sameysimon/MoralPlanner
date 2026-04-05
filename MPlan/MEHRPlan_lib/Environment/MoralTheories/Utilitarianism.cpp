
#include "Utilitarianism.hpp"
#include "QValue.hpp"

int MEHRUtilitarianism::attack(QValue& qv1, QValue& qv2) {
    double qv1_utils=0;
    double qv2_utils=0;
    for (size_t con_id : considerationIndex) {
        qv1_utils += static_cast<ExpectedUtility*>(qv1.expectations[con_id].get())->value;
        qv2_utils += static_cast<ExpectedUtility*>(qv2.expectations[con_id].get())->value;
    }
    if (qv1_utils < qv2_utils) {
        return -1;
    }
    if (qv1_utils > qv2_utils) {
        return 1;
    }
    return 0;
}
Attack MEHRUtilitarianism::CriticalQuestionOne(Attack& att, policy_hists& histories) {
    att.theoryIdx = mId;
    return pSortedHistories->CriticalQuestionOne(att, histories);
}


int MEHRUtilitarianism::CriticalQuestionTwo(QValue& qv1, QValue& qv2) {
    return attack(qv1, qv2);
}
