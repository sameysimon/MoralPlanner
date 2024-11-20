//
// Created by Simon Kolker on 29/08/2024.
//
#include <iostream>
#include <sstream>
#include "Solver.hpp"
#include <algorithm>
#include <numeric>

using namespace std;

std::string Solver::SolutionSetToString(std::vector<std::shared_ptr<Solution>>& solSet) {
    stringstream stream;
    stream << solSet.size() << " solutions." << endl;
    int solNum = 0;
    for (auto sol : solSet) {
        if (sol==nullptr) { stream << "Solution NULL" << solNum << endl; continue; }
        stream << "Solution #" << solNum << endl;
        //stream << sol->worthToString() << endl;
        stream << sol->policyToString() << endl;
        solNum++;
    }
    stream << endl;
    return stream.str();
}
std::string Solver::SolutionSetToString(unordered_set<shared_ptr<Solution>, SolutionHash, SolutionEqual>& solSet) {
    stringstream stream;
    stream << solSet.size() << " solutions." << endl;
    int solNum = 0;
    for (auto sol : solSet) {
        if (sol==nullptr) { stream << "Solution NULL" << solNum << endl; continue; }
        stream << "Solution #" << solNum << endl;
        stream << sol->worthToString() << endl;
        stream << sol->policyToString() << endl;
        solNum++;
    }
    stream << endl;
    return stream.str();
}



