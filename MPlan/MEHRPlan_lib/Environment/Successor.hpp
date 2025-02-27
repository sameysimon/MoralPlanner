//
//  Successor.hpp
//  MPlan
//
//  Created by e56834sk on 11/07/2024.
//

#ifndef Successor_hpp
#define Successor_hpp

#include <sstream>
#include <stdio.h>

class Successor {
public:
    int source;
    int target;
    double probability;
    Successor(int _sourceIdx, int _targetIdx, double _prob) : source(_sourceIdx), target(_targetIdx), probability(_prob){}
    std::string ToString() {
        std::stringstream stream;
        stream << "Source State " << source << " Target State " << target << " Probability " << probability;
        return stream.str();
    }
};


#endif /* Successor_hpp */
