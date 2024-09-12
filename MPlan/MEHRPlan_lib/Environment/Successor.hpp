//
//  Successor.hpp
//  MPlan
//
//  Created by e56834sk on 11/07/2024.
//

#ifndef Successor_hpp
#define Successor_hpp

#include <stdio.h>

class Successor {
public:
    int source;
    int target;
    double probability;
    double cost;
    Successor(int _sourceIdx, int _targetIdx, double _prob, double _cost) : source(_sourceIdx), target(_targetIdx), probability(_prob), cost(_cost) {}
    Successor() {}
};


#endif /* Successor_hpp */
