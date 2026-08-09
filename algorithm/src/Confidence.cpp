#include "../include/Confidence.hpp"

#include <algorithm>
#include <cmath>

namespace
{

HorizonConfidence calculateHorizonConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    std::size_t horizon,
    double threshold
)
{
    HorizonConfidence result{};

    if (weightedMatches.empty())
    {
        return result;
    }

    double positiveWeight = 0.0;
    double negativeWeight = 0.0;

    for (const auto& match : weightedMatches)
    {
        const std::size_t historicalWindow =
            match.windowIndex;

        const std::size_t endIndex =
            historicalWindow + windowSize - 1;

        const std::size_t futureIndex =
            endIndex + horizon;

        if (futureIndex >= prices.size())
        {
            continue;
        }

        const double currentPrice =
            prices[endIndex];

        if (currentPrice == 0.0)
        {
            continue;
        }

        const double futurePrice =
            prices[futureIndex];

        const double futureReturn =
            ((futurePrice - currentPrice)
             / currentPrice) * 100.0;

        if (futureReturn > 0.0)
        {
            positiveWeight +=
                match.normalizedWeight;
        }
        else if (futureReturn < 0.0)
        {
            negativeWeight +=
                match.normalizedWeight;
        }
    }

    const double totalDirectionalWeight =
        positiveWeight + negativeWeight;

    if (totalDirectionalWeight <= 0.0)
    {
        return result;
    }

    /*
        Normalize again in case any historical
        candidates were skipped.
    */

    positiveWeight /=
        totalDirectionalWeight;

    negativeWeight /=
        totalDirectionalWeight;

    result.positiveWeight =
        positiveWeight;

    result.negativeWeight =
        negativeWeight;

    /*
        Confidence = weight of the majority direction.
    */

    result.confidence =
        std::max(
            positiveWeight,
            negativeWeight
        );

    result.predictedPositive =
        positiveWeight >= negativeWeight;

    /*
        Only generate a signal when the
        historical matches agree strongly enough.
    */

    result.signal =
        result.confidence >= threshold;

    return result;
}

}

ConfidenceResult calculateConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    double threshold
)
{
    ConfidenceResult result{};

    result.confidence5 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            5,
            threshold
        );

    result.confidence10 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            10,
            threshold
        );

    result.confidence15 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            15,
            threshold
        );

    result.confidence30 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            30,
            threshold
        );

    return result;
}