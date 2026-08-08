#include "../include/PatternMatcher.hpp"

#include <cmath>
#include <algorithm>
#include <stdexcept>


double calculateDistance(
    const std::vector<double>& a,
    const std::vector<double>& b
)
{
    if (a.size() != b.size())
    {
        throw std::invalid_argument(
            "FFT signatures must have the same size"
        );
    }

    double sum = 0.0;

    for (std::size_t i = 0; i < a.size(); ++i)
    {
        double difference = a[i] - b[i];

        sum += difference * difference;
    }

    return std::sqrt(sum);
}


std::vector<PatternMatch> findTopMatches(
    const std::vector<double>& currentSignature,
    const std::vector<std::vector<double>>& historicalSignatures,
    std::size_t topK
)
{
    std::vector<PatternMatch> matches;

    matches.reserve(historicalSignatures.size());

    for (std::size_t i = 0; i < historicalSignatures.size(); ++i)
    {
        double distance = calculateDistance(
            currentSignature,
            historicalSignatures[i]
        );

        matches.push_back({
            i,
            distance
        });
    }
    std::sort(
        matches.begin(),
        matches.end(),
        [](const PatternMatch& a, const PatternMatch& b)
        {
            return a.distance < b.distance;
        }
    );

    if (matches.size() > topK)
    {
        matches.resize(topK);
    }

    return matches;
}