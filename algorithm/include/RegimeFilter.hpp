#ifndef REGIME_FILTER_HPP
#define REGIME_FILTER_HPP

#include <cstddef>
#include <vector>

#include "WeightedRanking.hpp"


std::vector<WeightedMatch> applyRegimeFilter(
    const std::vector<double>& prices,
    std::size_t currentEndIndex,
    std::size_t windowSize,
    const std::vector<WeightedMatch>& weightedMatches
);

#endif