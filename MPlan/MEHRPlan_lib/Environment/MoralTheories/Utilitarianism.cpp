#include "Utilitarianism.hpp"
#include "QValue.hpp"

int Utilitarianism::attack(QValue& qv1, QValue& qv2) {
    return qv1.expectations[id]->compare(*qv2.expectations[id]);
}