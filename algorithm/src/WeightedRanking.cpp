#include "../include/WeightedRanking.hpp"

#include <cmath>
#include <cstddef>

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

    const double EPSILON = 1e-9;

    double totalWeight = 0.0;

    for (std::size_t i = 0;
         i < distances.size();
         ++i)
    {
        double weight =
            1.0 / (distances[i] + EPSILON);

        result.push_back({
            windowIndices[i],
            distances[i],
            weight,
            0.0
        });

        totalWeight += weight;
    }

   
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