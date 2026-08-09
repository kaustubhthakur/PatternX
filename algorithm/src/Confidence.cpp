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

    if (weightedMatches.empty() ||
        windowSize == 0)
    {
        return result;
    }

    double positiveWeight = 0.0;
    double negativeWeight = 0.0;

    double positiveReturnWeight = 0.0;
    double negativeReturnWeight = 0.0;

    for (const auto& match : weightedMatches)
    {
        const std::size_t historicalWindow =
            match.windowIndex;

        if (historicalWindow >= prices.size())
        {
            continue;
        }

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
            ((futurePrice - currentPrice) /
             currentPrice) * 100.0;

        const double weight =
            match.normalizedWeight;

        if (futureReturn > 0.0)
        {
            positiveWeight += weight;
        }
        else if (futureReturn < 0.0)
        {
            negativeWeight += weight;
        }

        /*
            Expected-return-aware confidence.

            Only returns whose magnitude reaches the minimum
            meaningful threshold contribute to the directional
            return buckets.
        */
        if (futureReturn >= minimumExpectedReturn)
        {
            positiveReturnWeight += weight;
        }
        else if (futureReturn <= -minimumExpectedReturn)
        {
            negativeReturnWeight += weight;
        }
    }

    const double totalDirectionalWeight =
        positiveWeight + negativeWeight;

    if (totalDirectionalWeight <= 0.0)
    {
        return result;
    }

    positiveWeight /=
        totalDirectionalWeight;

    negativeWeight /=
        totalDirectionalWeight;

    result.positiveWeight =
        positiveWeight;

    result.negativeWeight =
        negativeWeight;

    /*
        Base confidence is the weight of the majority
        direction.
    */
    result.confidence =
        std::max(
            positiveWeight,
            negativeWeight
        );

    result.predictedPositive =
        positiveWeight >= negativeWeight;

    /*
        If there is meaningful-return evidence available,
        use it to prevent tiny positive/negative moves from
        creating an overly strong signal.
    */
    const double meaningfulTotal =
        positiveReturnWeight +
        negativeReturnWeight;

    if (meaningfulTotal > 0.0)
    {
        positiveReturnWeight /=
            meaningfulTotal;

        negativeReturnWeight /=
            meaningfulTotal;

        const double meaningfulConfidence =
            std::max(
                positiveReturnWeight,
                negativeReturnWeight
            );

        /*
            Combine the ordinary directional agreement with
            meaningful-return agreement.
        */
        result.confidence =
            std::min(
                result.confidence,
                meaningfulConfidence
            );

        result.predictedPositive =
            positiveReturnWeight >=
            negativeReturnWeight;
    }

    result.signal =
        result.confidence >= threshold;

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