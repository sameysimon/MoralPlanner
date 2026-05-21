//
// Created by Simon Kolker on 07/04/2026.
//

#pragma once
#include <vector>
#include "Policy.hpp"


class AAF_Convert {
    vector<string> arguments;
    vector<pair<string, string>> attacks;
public:
    AAF_Convert(std::vector<std::unique_ptr<Policy>> &policies, vector<vector<unique_ptr<History>>> &histories, MDP& mdp) {
        for (size_t th_idx =0; th_idx < mdp.mehr_theories.size(); ++th_idx) {
            for (size_t pi_idx =0; pi_idx < policies.size(); ++pi_idx) {
                for (size_t h_idx =0; h_idx < histories.size(); ++h_idx) {
                    arguments.push_back(std::format("th{}_pi{}_hi{}", th_idx, pi_idx, h_idx));
                }
            }
        }
        /*
        for (size_t th_idx =0; th_idx < mdp.mehr_theories.size(); ++th_idx) {
            for (size_t pi_idx_1 =0; pi_idx_1 < policies.size(); ++pi_idx_1) {
                for (size_t pi_idx_2 =0; pi_idx_2 < policies.size(); ++pi_idx_2) {
                    auto m = mdp.mehr_theories[th_idx];
                    auto x = m->CriticalQuestionTwo(*policies[pi_idx_1]->getExpectationPtr(), *policies[pi_idx_2]->getExpectationPtr());

                    if (x==1 && m->CriticalQuestionOne()) {

                    }
                }
            }

        }*/
    }
};

