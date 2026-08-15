#pragma once

#include <vector>
#include <cstddef>
#include "WeightedRanking.hpp"   

struct HorizonConfidence
{
    double confidence = 0.0;
    double positiveWeight = 0.0;
    double negativeWeight = 0.0;
    bool predictedPositive = false;
    bool signal = false;
};

struct ConfidenceResult
{
    HorizonConfidence confidence5;
    HorizonConfidence confidence10;
    HorizonConfidence confidence15;
    HorizonConfidence confidence30;
};

ConfidenceResult calculateConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    double threshold,
    double minimumExpectedReturn = 0.0
);