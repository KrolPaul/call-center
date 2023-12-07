#pragma once

#include <random>
#include <ctime>

namespace rndgen{

size_t genRandUniform(const size_t min, const size_t max);

};


inline size_t rndgen::genRandUniform(const size_t min, const size_t max){
    std::mt19937 gen(time(NULL));
    // Distribute results between inclusive
    std::uniform_int_distribution<std::mt19937::result_type> dist(min, max); 
    return dist(gen);
}