//
// Created by Simon Kolker on 23/04/2025.
//
#pragma once
#include "MoralTheory.hpp"
#include "Successor.hpp"
#include "State.hpp"
#include "Utilitarianism.hpp"
#include "unordered_set"

class MEHRMaximin : public MEHRTheory {
    std::vector<size_t> considerations;
    // Stores attacked histories for each policy. attacks[policy_idx] = [attacked histories]
    std::vector<std::unordered_set<size_t>> attacks;
public:
    MEHRMaximin(size_t rank_, size_t id_, std::string &name_) : MEHRTheory(rank_, id_, name_) { }
    void AddConsideration(Consideration& con) override { considerations.push_back(con.id); }
    int attack(QValue& qv1, QValue& qv2) override;
    Attack CriticalQuestionOne(Attack& a, std::vector<std::vector<History*>> &histories) override;
    int CriticalQuestionTwo(QValue& qv1, QValue& qv2) override;
    void InitMEHR(std::vector<std::vector<History*>> &histories) override;
    void AddPoliciesForMEHR(std::vector<std::vector<History*>> &histories) override;
};