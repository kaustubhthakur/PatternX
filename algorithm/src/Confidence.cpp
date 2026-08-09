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
    double threshold,
    double minimumExpectedReturn
)
{
    HorizonConfidence result{};

    if (weightedMatches.empty())
    {
        return result;
    }

    double positiveWeight = 0.0;
    double negativeWeight = 0.0;
    double predictedReturn = 0.0;

    for (const auto& match : weightedMatches)
    {
        const std::size_t historicalWindow =
            match.windowIndex;

        /*
            The historical window ends here.
        */
        const std::size_t endIndex =
            historicalWindow + windowSize - 1;

        if (endIndex >= prices.size())
        {
            continue;
        }

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

        /*
            Weighted expected return.
        */
        predictedReturn +=
            match.normalizedWeight * futureReturn;

        /*
            Directional confidence.
        */
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
        Normalize again because some historical candidates
        may have been skipped due to unavailable future data.
    */
    positiveWeight /=
        totalDirectionalWeight;

    negativeWeight /=
        totalDirectionalWeight;

    result.positiveWeight =
        positiveWeight;

    result.negativeWeight =
        negativeWeight;

    result.predictedReturn =
        predictedReturn;

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
        Signal now requires BOTH:
          1. sufficient directional confidence
          2. sufficient expected return magnitude

        Example:
          confidence = 75%
          predicted return = +0.10%
          minimum expected return = 0.50%
          => NO SIGNAL
    */
    result.signal =
        result.confidence >= threshold &&
        std::abs(result.predictedReturn) >=
            minimumExpectedReturn;

    return result;
}

} // namespace


ConfidenceResult calculateConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    double threshold,
    double minimumExpectedReturn
)
{
    ConfidenceResult result{};

    result.confidence5 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            5,
            threshold,
            minimumExpectedReturn
        );

    result.confidence10 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            10,
            threshold,
            minimumExpectedReturn
        );

    result.confidence15 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            15,
            threshold,
            minimumExpectedReturn
        );

    result.confidence30 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            30,
            threshold,
            minimumExpectedReturn
        );

    return result;
}