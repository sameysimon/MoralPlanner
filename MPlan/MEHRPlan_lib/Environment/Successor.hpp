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
    double long probability;
    Successor(int _sourceIdx, int _targetIdx, double _prob) : source(_sourceIdx), target(_targetIdx), probability(_prob){}
    Successor() {}
};


#endif /* Successor_hpp */
