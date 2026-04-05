


#include "Solver.hpp"

vector<QValue> Solver::EvaluatePolicy(Policy& pi, size_t until_time) {
  auto data = build_blank_data(false, until_time);


  // Get reachable states
  vector<size_t> stateOrder;
  vector<size_t> stack;
  unordered_set<size_t> visited;
  stack.push_back(0);
  while (!stack.empty()) {
    size_t stateIdx = stack.back();
    stack.pop_back();
    if (visited.find(stateIdx) != visited.end()) {
      continue;
    }
    visited.insert(stateIdx);
    if (mdp.states[stateIdx]->time >= until_time) {
      continue;
    }
    auto stateAction = pi.policy[static_cast<int>(stateIdx)];
    auto scrs = mdp.getActionSuccessors(*mdp.states[stateIdx], stateAction);
    if (scrs==nullptr) { continue; }
    for (auto scr : *scrs) {
      visited.insert(stateIdx);
      stateOrder.push_back(stateIdx);
      stack.push_back(stateIdx);
    }
  }
  // Sort in reverse-order
  sort(stateOrder.begin(), stateOrder.end(), [this](size_t l, size_t r) {return mdp.states[l]->time > mdp.states[r]->time;});
  // Build Policy Evaluation

  for (auto stateIdx : stateOrder) {
    candidates.clear();
    indicesOfUndominated.clear();
    qValueIdxToAction.clear();
    // Get the values
    getUnDomCandidates(*mdp.states[stateIdx], candidates, indicesOfUndominated, qValueIdxToAction);
    if (indicesOfUndominated.empty()) { continue; }
    // Update Data values to current undominated.
    data.at(stateIdx).clear();
    for (auto elem : indicesOfUndominated) {
      data.at(stateIdx).push_back(candidates[elem]);
    }
  }
  return data[0];
}