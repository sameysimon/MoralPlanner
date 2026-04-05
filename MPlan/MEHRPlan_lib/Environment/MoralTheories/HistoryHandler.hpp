//
// Created by Simon Kolker on 29/12/2025.
//

#pragma once
#include <vector>
#include "MoralTheory.hpp"
using namespace std;


class HistoryHandler {
protected:
    MEHRTheory &rMehrTheory;
public:
    explicit HistoryHandler(MEHRTheory &mehrTheory) : rMehrTheory(mehrTheory) { }
    virtual ~HistoryHandler() = default;
    virtual void InitMEHR(policy_hists &histories) = 0;
    virtual void AddPolicyHistories(policy_hists& histories) = 0;
};

class SortHistories : public HistoryHandler {
public:
    std::vector<std::vector<size_t>> orderedHistories;
    explicit SortHistories(MEHRTheory &mehrTheory): HistoryHandler(mehrTheory) {}
    void InitMEHR(policy_hists &histories) override;
    void AddPolicyHistories(policy_hists& histories) override;

    Attack CriticalQuestionOne(Attack& a, policy_hists& histories);
};
