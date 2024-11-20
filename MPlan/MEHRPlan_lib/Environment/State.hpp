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
    bool isGoal;
    std::string tag;
    std::vector<std::vector<Successor*>*> actionSuccessors;

    State(int _index, int action_reserve)  {
        id = _index;
        actionSuccessors = std::vector<std::vector<Successor*>*>(action_reserve);
        this->isGoal = false;
    }
    ~State() {
        for (std::vector<Successor*>* successorCollection : actionSuccessors) {
            delete successorCollection;
        }
    }
};


#endif /* State_hpp */
