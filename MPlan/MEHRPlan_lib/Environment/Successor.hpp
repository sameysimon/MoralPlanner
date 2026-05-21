//
//  Successor.hpp
//  MPlan
//
//  Created by e56834sk on 11/07/2024.
//

#pragma once

#include <sstream>
#include <string>
#include <functional>

class Successor {
public:
    int source;
    int target;
    double probability;
    size_t action_idx;

    Successor(int _sourceIdx,
              int _targetIdx,
              double _prob,
              size_t _action_idx)
        : source(_sourceIdx),
          target(_targetIdx),
          probability(_prob),
          action_idx(_action_idx) {}

    std::string ToString() const {
        std::stringstream stream;
        stream << "Source State " << source
               << " Target State " << target
               << " Probability " << probability;
        return stream.str();
    }
};

class SuccessorHash {
public:
    std::size_t operator()(const Successor* scr) const noexcept {
        size_t h = std::hash<int>()(scr->source);

        h ^= std::hash<int>()(scr->target)
             + 0x9e3779b9 + (h << 6) + (h >> 2);

        h ^= std::hash<double>()(scr->probability)
             + 0x9e3779b9 + (h << 6) + (h >> 2);

        h ^= std::hash<size_t>()(scr->action_idx)
             + 0x9e3779b9 + (h << 6) + (h >> 2);

        return h;
    }
};

class SuccessorEqual {
public:
    bool operator()(const Successor* lhs,
                    const Successor* rhs) const noexcept {
        return lhs->source == rhs->source &&
               lhs->target == rhs->target &&
               abs(lhs->probability - rhs->probability) < 0.000001 &&
               lhs->action_idx == rhs->action_idx;
    }
};

