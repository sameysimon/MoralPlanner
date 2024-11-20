//
//  Solution.cpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//
#include "Solution.hpp"
#include <sstream>
using namespace std;

string Solution::policyToString() {
    stringstream stream;
    stream << "***Policy Table (x-time, y-state)***" << endl;
    stream << "T: ";
    for (int t = 0; t < mdp->horizon; t++) {
        stream << t << " | ";
    }
    stream << endl;
    for (int s = 0; s < mdp->states.size(); s++) {
        stream << s << ": ";
        for (int t = 0; t < mdp->horizon; t++) {
            if (policy[t][s] == -1) {
                stream << "_" << " | ";
                continue;
            }
            int stateActionIdx = policy[t][s];
            auto actions = mdp->getActions(*mdp->states[s],0);
            auto a = actions->at(stateActionIdx);
            stream << *(a->label) << " | ";
        }
        stream << endl;
    }
    return stream.str();
}

string Solution::worthToString() {
    std::stringstream stream;
    stream << "***Solution Table (x-time, y-state)***" << endl;
    stream << "T: ";
    for (int t = 0; t < mdp->horizon; t++) {
        stream << t << " | ";
    }
    stream << endl;
    for (int s = 0; s < mdp->states.size(); s++) {
        stream << s << ": ";
        for (int t = 0; t < mdp->horizon; t++) {
            for (int thIdx = 0; thIdx < expecters.size(); thIdx++) {
                stream << expecters[thIdx]->expectations[t][s]->ToString();
                if (t < expecters.size() - 1) {
                    stream << ", ";
                }
            }
            stream <<" | ";

        }
        stream << endl;
    }
    return stream.str();
}