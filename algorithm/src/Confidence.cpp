#include "../include/Confidence.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace
{

constexpr double EPSILON = 1e-12;

HorizonConfidence calculateHorizonConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    std::size_t horizon,
    double threshold,
    double minimumExpectedReturn,
    double typicalStdDev
)
{
    HorizonConfidence result{};

    if (prices.empty() ||
        weightedMatches.empty() ||
        windowSize == 0 ||
        horizon == 0)
    {
        return result;
    }

    double positiveWeight = 0.0;
    double negativeWeight = 0.0;

    double meaningfulPositiveWeight = 0.0;
    double meaningfulNegativeWeight = 0.0;

    std::vector<std::pair<double, double>> weightedReturns;
    weightedReturns.reserve(weightedMatches.size());

    for (const auto& match : weightedMatches)
    {
        const std::size_t start = match.windowIndex;

        if (start >= prices.size())
            continue;

        if (windowSize > prices.size() - start)
            continue;

        const std::size_t end =
            start + windowSize - 1;

        if (horizon > prices.size() - 1 - end)
            continue;

        const double currentPrice = prices[end];

        if (std::abs(currentPrice) <= EPSILON)
            continue;

        const std::size_t futureIndex =
            end + horizon;

        const double futurePrice =
            prices[futureIndex];

        const double futureReturn =
            ((futurePrice - currentPrice) /
             currentPrice) *
            100.0;

        const double weight =
            std::max(0.0, match.normalizedWeight);

        if (weight <= EPSILON)
            continue;

        weightedReturns.push_back(
            std::make_pair(weight, futureReturn)
        );

        if (futureReturn > EPSILON)
        {
            positiveWeight += weight;
        }
        else if (futureReturn < -EPSILON)
        {
            negativeWeight += weight;
        }

        if (futureReturn >= minimumExpectedReturn)
        {
            meaningfulPositiveWeight += weight;
        }
        else if (futureReturn <= -minimumExpectedReturn)
        {
            meaningfulNegativeWeight += weight;
        }
    }

    const double totalDirectionalWeight =
        positiveWeight + negativeWeight;

    if (totalDirectionalWeight <= EPSILON)
        return result;

    positiveWeight /= totalDirectionalWeight;
    negativeWeight /= totalDirectionalWeight;

    result.positiveWeight = positiveWeight;
    result.negativeWeight = negativeWeight;

    result.confidence =
        std::max(positiveWeight, negativeWeight);

    result.predictedPositive =
        positiveWeight >= negativeWeight;

    const double meaningfulTotal =
        meaningfulPositiveWeight +
        meaningfulNegativeWeight;

    if (minimumExpectedReturn > 0.0 &&
        meaningfulTotal > EPSILON)
    {
        meaningfulPositiveWeight /= meaningfulTotal;
        meaningfulNegativeWeight /= meaningfulTotal;

        const double meaningfulConfidence =
            std::max(
                meaningfulPositiveWeight,
                meaningfulNegativeWeight
            );

        result.confidence =
            std::min(
                result.confidence,
                meaningfulConfidence
            );

        result.predictedPositive =
            meaningfulPositiveWeight >=
            meaningfulNegativeWeight;
    }

    if (weightedReturns.size() >= 2 &&
        typicalStdDev > EPSILON)
    {
        double weightedMean = 0.0;
        double weightSum = 0.0;

        for (const auto& item : weightedReturns)
        {
            const double weight = item.first;
            const double value = item.second;

            weightedMean += weight * value;
            weightSum += weight;
        }

        if (weightSum > EPSILON)
        {
            weightedMean /= weightSum;

            double variance = 0.0;

            for (const auto& item : weightedReturns)
            {
                const double weight = item.first;
                const double value = item.second;

                const double difference =
                    value - weightedMean;

                variance +=
                    weight *
                    difference *
                    difference;
            }

            variance /= weightSum;

            const double standardDeviation =
                std::sqrt(
                    std::max(0.0, variance)
                );

            const double dispersionRatio =
                standardDeviation / typicalStdDev;

            const double dispersionPenalty =
                1.0 /
                (1.0 + dispersionRatio / 2.0);

            result.confidence =
                (0.7 * result.confidence) +
                (0.3 *
                 result.confidence *
                 dispersionPenalty);
        }
    }

    result.confidence =
        std::max(
            0.0,
            std::min(1.0, result.confidence)
        );

    result.signal =
        result.confidence >= threshold;

    return result;
}

}

double calculateHorizonTypicalStdDev(
    const std::vector<double>& prices,
    std::size_t horizon,
    std::size_t cutoffIndex
)
{
    if (prices.empty() || horizon == 0)
        return 0.0;

    cutoffIndex =
        std::min(cutoffIndex, prices.size());

    if (cutoffIndex <= horizon)
        return 0.0;

    std::vector<double> returns;

    returns.reserve(cutoffIndex - horizon);

    for (std::size_t i = 0;
         i + horizon < cutoffIndex;
         ++i)
    {
        const double currentPrice = prices[i];

        if (std::abs(currentPrice) <= EPSILON)
            continue;

        const double futurePrice =
            prices[i + horizon];

        const double value =
            ((futurePrice - currentPrice) /
             currentPrice) *
            100.0;

        returns.push_back(value);
    }

    if (returns.size() < 2)
        return 0.0;

    double mean = 0.0;

    for (const double value : returns)
        mean += value;

    mean /= static_cast<double>(returns.size());

    double variance = 0.0;

    for (const double value : returns)
    {
        const double difference =
            value - mean;

        variance +=
            difference * difference;
    }

    variance /=
        static_cast<double>(returns.size());

    return std::sqrt(
        std::max(0.0, variance)
    );
}

ConfidenceResult calculateConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    double threshold,
    double minimumExpectedReturn,
    std::size_t cutoffIndex
)
{
    ConfidenceResult result{};

    if (prices.empty() ||
        weightedMatches.empty())
    {
        return result;
    }

    if (cutoffIndex == 0 ||
        cutoffIndex > prices.size())
    {
        cutoffIndex = prices.size();
    }

    const double typicalStdDev5 =
        calculateHorizonTypicalStdDev(
            prices,
            5,
            cutoffIndex
        );

    const double typicalStdDev10 =
        calculateHorizonTypicalStdDev(
            prices,
            10,
            cutoffIndex
        );

    const double typicalStdDev15 =
        calculateHorizonTypicalStdDev(
            prices,
            15,
            cutoffIndex
        );

    const double typicalStdDev30 =
        calculateHorizonTypicalStdDev(
            prices,
            30,
            cutoffIndex
        );

    result.confidence5 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            5,
            threshold,
            minimumExpectedReturn,
            typicalStdDev5
        );

    result.confidence10 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            10,
            threshold,
            minimumExpectedReturn,
            typicalStdDev10
        );

    result.confidence15 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            15,
            threshold,
            minimumExpectedReturn,
            typicalStdDev15
        );

    result.confidence30 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            30,
            threshold,
            minimumExpectedReturn,
            typicalStdDev30
        );

    return result;
}