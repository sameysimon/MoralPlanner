//
//  Action.hpp
//  MPlan
//
//  Created by e56834sk on 11/07/2024.
//

#ifndef Action_hpp
#define Action_hpp

#include <stdio.h>
#include <string>

class Action {
public:
    std::string* label;
    int id;
    Action(int _id, std::string* _label) : id(_id), label(_label) {};
    ~Action() = default;
};

#endif /* Action_hpp */
