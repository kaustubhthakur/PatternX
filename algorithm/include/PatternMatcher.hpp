#ifndef PATTERN_MATCHER_HPP
#define PATTERN_MATCHER_HPP

#include <cstddef>
#include <vector>

struct PatternMatch
{
    std::size_t windowIndex;

    // Original FFT distance
    double fftDistance;

    // Trend similarity distance
    double trendDistance;

    // Final combined distance
    double combinedDistance;
};

double calculateDistance(
    const std::vector<double>& a,
    const std::vector<double>& b
);

double calculateTrendDistance(
    const std::vector<double>& currentWindow,
    const std::vector<double>& historicalWindow
);

std::vector<PatternMatch> findTopMatches(
    const std::vector<double>& currentSignature,
    const std::vector<std::vector<double>>& historicalSignatures,
    const std::vector<double>& currentWindow,
    const std::vector<std::vector<double>>& historicalWindows,
    std::size_t currentIndex,
    std::size_t topK,
    std::size_t minimumSeparation
);

#endif