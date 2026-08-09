#ifndef WEIGHTED_RANKING_HPP
#define WEIGHTED_RANKING_HPP

#include <cstddef>
#include <vector>

struct WeightedMatch
{
    std::size_t windowIndex;
    double distance;
    double weight;
    double normalizedWeight;
};

std::vector<WeightedMatch> calculateWeights(
    const std::vector<std::size_t>& windowIndices,
    const std::vector<double>& distances
);

#endif