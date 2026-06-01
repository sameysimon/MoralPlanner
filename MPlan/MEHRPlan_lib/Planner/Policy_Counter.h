//
// Created by psiko on 6/1/26.
//

#pragma once
#include <unordered_set>
#include "MDP.hpp"


using namespace std;
class CountPolicies {
private:
    MDP* pMdp;
    vector<__int128> memo;
    __int128 CountAtPair(size_t state_idx) {
        if (pMdp->states[state_idx]->time == pMdp->horizon - 1) {
            return 1;
        }
        if (memo[state_idx] != -1) {
            return memo[state_idx];
        }
        auto *acts = pMdp->getActions(*pMdp->states[state_idx]);
        if (acts->empty()) {
            return 1;
        }
        __int128 total = 0;
        for (int a_idx = 0; a_idx < acts->size(); a_idx++) {
            __int128 prod = 1;
            auto *scrs = pMdp->getActionSuccessors(state_idx, a_idx);
            for (auto *scr : *scrs) {
                prod *= CountAtPair(scr->target);
            }
            total += prod;
        }

        memo[state_idx] = total;
        return total;
    }
public:
    CountPolicies(MDP& mdp) {
        pMdp = &mdp;
        memo = vector<__int128>(mdp.states.size(), -1);
    }
    __int128 GetCount() {
        return CountAtPair(0);
    }
};
