//
// Created by Simon Kolker on 30/08/2024.
//

#ifndef MEHR_H
#define MEHR_H
#include "Solution.hpp"
#include "Attack.hpp"

using namespace std;

class MEHR {
  public:
    vector<int> findNonAccept(vector<QValue*>& expectedWorths, vector<vector<QValue*>>&& outcomeWorths, vector<vector<double>>&& probability);
    Attack biRetrospect(QValue* outOne, QValue* expOne, double probOne, QValue* outTwo, QValue* expTwo, double probTwo);

    void verifyInput(vector<QValue*>& expOne, vector<vector<QValue*>>&& outsOne, vector<vector<double>>&& probOne,
        vector<QValue*>& expTwo, vector<vector<QValue*>>& outsTwo, vector<vector<double>>& probTwo) {
        if (expOne.size() != outsOne.size() || outsOne.size() != probOne.size())
            throw runtime_error("MEHR::verifyInput size mismatch between expectations and arguments/probabilities");
        if (expTwo.size() != outsTwo.size() || outsTwo.size() != probTwo.size())
            throw runtime_error("MEHR::verifyInput size mismatch between expectations and arguments/probabilities");

        for (int i = 0; i < outsOne.size(); i++) {
            if (outsOne[i].size() != probOne[i].size()) {
                throw runtime_error("MEHR::verifyInput size mismatch between arguments and probabilities");
            }
        }
        for (int i = 0; i < outsTwo.size(); i++) {
            if (outsOne[i].size() != probTwo[i].size()) {
                throw runtime_error("MEHR::verifyInput size mismatch between arguments and probabilities");
            }
        }
    }




};



#endif //MEHR_H
