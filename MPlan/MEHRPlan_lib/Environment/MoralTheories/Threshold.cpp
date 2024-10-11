//
// Created by Simon Kolker on 01/10/2024.
//

#include "Threshold.hpp"
#include "QValue.hpp"

int Threshold::attack(QValue& qv1, QValue& qv2) {
    auto val_1 = static_cast<ExpectedValue*>(qv1.expectations[id]);
    auto val_2 = static_cast<ExpectedValue*>(qv2.expectations[id]);

    if (val_1->value >= threshold and val_2->value < threshold) {
        return 1;
    }
    if (val_1->value < threshold and val_2->value >= threshold) {
        return -1;
    }
    return 0;
}