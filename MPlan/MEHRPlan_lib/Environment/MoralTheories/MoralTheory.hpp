//
//  MoralTheory.hpp
//  MPlan
//
//  Created by e56834sk on 29/07/2024.
//

#ifndef MoralTheory_hpp
#define MoralTheory_hpp

#include <string>
#include <typeindex>
#include <nlohmann/json.hpp>
#include "Successor.hpp"

class Expecter;

class WorthBase {
public:
//    MoralTheory* theory;
    int test=0;
    virtual std::string ToString() const = 0;
    virtual int compare(WorthBase& wb) const = 0;
    virtual bool isEquivalent(WorthBase& w) const = 0;
    virtual WorthBase* clone() const = 0;
    virtual WorthBase& operator=(WorthBase& w) = 0;
    virtual ~WorthBase() = default;
};


class MoralTheory {
public:
    int rank;
    std::string label;
    //
    virtual WorthBase* gather(std::vector<Successor*>& successors, std::vector<WorthBase*>& expectations) = 0;
    // Setup
    virtual WorthBase* newExpectation() = 0;
    virtual void processSuccessor(Successor* successor, nlohmann::json successorData) = 0;
    Expecter* makeExpecter(int size, int horizon);
    virtual ~MoralTheory() = default;

};



class Value {
public:
    MoralTheory* theory;
};










#endif /* MoralTheory_hpp */
