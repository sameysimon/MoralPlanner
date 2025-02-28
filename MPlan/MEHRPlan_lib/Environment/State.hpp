//
//  State.hpp
//  MPlan
//
//  Created by e56834sk on 11/07/2024.
//

#ifndef State_hpp
#define State_hpp

#include <stdio.h>
#include <vector>
#include "Successor.hpp"
#include "Action.hpp"
#include <unordered_map>

class State {
public:
    int id;
    int time;
    bool isGoal;
    bool hasSuccessors = false;
    std::string tag;
    std::vector<std::vector<Successor*>*> actionSuccessors;

    State(int _index, int action_reserve, int _time)  {
        id = _index;
        time = _time;
        actionSuccessors = std::vector<std::vector<Successor*>*>(action_reserve);
        this->isGoal = false;
    }
    ~State() {
        for (std::vector<Successor*>* successorCollection : actionSuccessors) {
            for (Successor* successor : *successorCollection) {
                delete successor;
            }
            delete successorCollection;
        }
    }
    std::vector<Successor*>* addAction(const std::string& actionLabel) {
        auto* scrSet = new std::vector<Successor*>();
        actionSuccessors[actionCount] = scrSet;
        actionCount++;
        return scrSet;
    }
private:
    int actionCount = 0;
};


#endif /* State_hpp */
