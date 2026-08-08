#ifndef PATTERN_MATCHER_HPP
#define PATTERN_MATCHER_HPP

#include <vector>
#include <cstddef>

struct PatternMatch
{
    std::size_t windowIndex;
    double distance;
};

double calculateDistance(
    const std::vector<double>& a,
    const std::vector<double>& b
);

std::vector<PatternMatch> findTopMatches(
    const std::vector<double>& currentSignature,
    const std::vector<std::vector<double>>& historicalSignatures,
    std::size_t topK
);

#endif