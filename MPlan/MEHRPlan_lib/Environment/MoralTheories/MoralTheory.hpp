//
//  MoralTheory.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#ifndef MoralTheory_hpp
#define MoralTheory_hpp

#include <string>
#include <nlohmann/json.hpp>
#include "Successor.hpp"

class Expecter;
class QValue;
class State;

class WorthBase {
public:
    int test=0;
    virtual std::string ToString() const = 0;
    virtual int compare(WorthBase& wb) const = 0;
    virtual bool isEquivalent(WorthBase& w) const = 0;
    virtual WorthBase* clone() const = 0;
    virtual WorthBase& operator=(WorthBase& w) = 0;
    virtual ~WorthBase() = default;
    virtual std::size_t hash() = 0;
};

class MoralTheory {
public:
    int rank;
    std::string label;
    virtual ~MoralTheory() = default;
    //
    virtual WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability=false) = 0;
    // Setup
    virtual WorthBase* newWorth() = 0;
    virtual WorthBase* newHeuristic(State& s) = 0;
    virtual void processSuccessor(Successor* successor, nlohmann::json successorData) = 0;
    virtual void processHeuristics(nlohmann::json& heuristicData) = 0;
    Expecter* makeExpecter(int size, int horizon);
    Expecter* makeHeuristicExpecter(std::vector<State*>& states, int horizon);




};


#endif /* MoralTheory_hpp */
