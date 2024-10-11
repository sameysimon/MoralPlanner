//
// Created by Simon Kolker on 11/10/2024.
//

#include "Absolutism.hpp"
#include "QValue.hpp"

int Absolutism::attack(QValue& qv1, QValue& qv2) {
    return qv1.expectations[id]->compare(*qv2.expectations[id]);
}