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
    virtual void InitMEHR(std::vector<std::vector<History*>> &histories) = 0;
    virtual void AddPolicyHistories(std::vector<std::vector<History*>> &histories) = 0;
};

class SortHistories : public HistoryHandler {
public:
    std::vector<std::vector<size_t>> orderedHistories;
    explicit SortHistories(MEHRTheory &mehrTheory): HistoryHandler(mehrTheory) {}
    void InitMEHR(std::vector<std::vector<History*>> &histories) override;
    void AddPolicyHistories(std::vector<std::vector<History*>> &histories) override;

    Attack CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>>& histories);
};
