//
// Created by Simon Kolker on 30/08/2024.
//

#include "MEHR.hpp"
/*
vector<int> MEHR::findNonAccept(vector<QValue*>& expectedWorths, vector<vector<QValue*>>&& outcomeWorths, vector<vector<double>>&& probability) {
    vector<int> non_accept = vector<int>(expectedWorths.size(), 0);
    for (int oneIdx=0; oneIdx<expectedWorths.size(); ++oneIdx) {
        for (int twoIdx=oneIdx; twoIdx<expectedWorths.size(); ++twoIdx) {
            QValue* oneExp = expectedWorths[oneIdx];
            QValue* twoExp = expectedWorths[twoIdx];
            for (int outcomeOneIdx=0; outcomeOneIdx<outcomeWorths[oneIdx].size(); ++outcomeOneIdx) {
                QValue* outcomeOne = outcomeWorths[oneIdx][outcomeOneIdx];
                double probOne = probability[oneIdx][outcomeOneIdx];
                for (int outcomeTwoIdx=0; outcomeOneIdx<outcomeWorths[twoIdx].size(); ++outcomeTwoIdx) {
                    QValue* outcomeTwo = outcomeWorths[twoIdx][outcomeTwoIdx];
                    double probTwo = probability[twoIdx][outcomeTwoIdx];
                    Attack result = biRetrospect(outcomeOne, oneExp, probOne, outcomeTwo, twoExp, probTwo);
                    if (result==Attack::FORWARD || result==Attack::DILEMMA) {
                        // add non-accept probTwo to expTwo non-accept.
                    } else if (result==Attack::BACKWARD || result==Attack::DILEMMA) {
                        // add non-accept probOne to expOne non-accept.
                    }


                }
            }
        }
    }

    return non_accept;
}

Attack MEHR::biRetrospect(QValue* outOne, QValue* expOne, double probOne, QValue* outTwo, QValue* expTwo, double probTwo) {
    return Attack::FORWARD;
}
*/