//
//  MoralTheory.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#ifndef MoralTheory_hpp
#define MoralTheory_hpp
#include <sstream>
#include <iomanip>
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
    int id;
    MoralTheory(int id_) : id(id_) {}
    virtual ~MoralTheory() = default;
    //
    virtual WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& baselines, bool ignoreProbability=false) = 0;
    // Setup
    virtual WorthBase* newWorth() = 0;
    virtual WorthBase* newHeuristic(State& s) = 0;
    virtual void processSuccessor(Successor* successor, nlohmann::json successorData) = 0;
    virtual void processHeuristics(nlohmann::json& heuristicData) = 0;
    virtual int attack(QValue& qv1, QValue& qv2) = 0;
    Expecter* makeExpecter(int size, int horizon);
    Expecter* makeHeuristicExpecter(std::vector<State*>& states, int horizon);




};

static std::string doubleToString(double long v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(15) << v; // Convert with maximum precision
    std::string str = oss.str();

    // Strip trailing zeros
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    // If there's a decimal point left at the end, remove it
    if (str.back() == '.') {
        str.pop_back();
    }
    return str;
}

#endif /* MoralTheory_hpp */
