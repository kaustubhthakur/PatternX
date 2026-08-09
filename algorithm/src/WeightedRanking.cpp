#include "../include/WeightedRanking.hpp"

#include <cmath>

std::vector<WeightedMatch> calculateWeights(
    const std::vector<std::size_t>& windowIndices,
    const std::vector<double>& distances
)
{
    std::vector<WeightedMatch> result;

    if (windowIndices.size() != distances.size())
    {
        return result;
    }

    if (distances.empty())
    {
        return result;
    }

    // Controls how strongly distance affects the weight.
    // Higher alpha = stronger preference for closer matches.
    const double ALPHA = 5.0;

    double totalWeight = 0.0;

    for (std::size_t i = 0;
         i < distances.size();
         ++i)
    {
        double distance = distances[i];

        // Exponential distance weighting
        double weight =
            std::exp(-ALPHA * distance);

        result.push_back({
            windowIndices[i],
            distance,
            weight,
            0.0
        });

        totalWeight += weight;
    }

    // Normalize weights so that their sum = 1
    if (totalWeight > 0.0)
    {
        for (auto& match : result)
        {
            match.normalizedWeight =
                match.weight / totalWeight;
        }
    }

    return result;
}