#include "../include/Confidence.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

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

    if (weightedMatches.empty() || windowSize == 0)
    {
        return result;
    }

    double positiveWeight = 0.0;
    double negativeWeight = 0.0;

    double positiveReturnWeight = 0.0;
    double negativeReturnWeight = 0.0;

    // NEW: collect (weight, return) pairs to measure dispersion
    std::vector<std::pair<double, double>> weightedReturns;
    weightedReturns.reserve(weightedMatches.size());

    for (const auto& match : weightedMatches)
    {
        const std::size_t historicalWindow = match.windowIndex;

        if (historicalWindow >= prices.size())
            continue;

        const std::size_t endIndex = historicalWindow + windowSize - 1;
        if (endIndex >= prices.size())
            continue;

        const std::size_t futureIndex = endIndex + horizon;
        if (futureIndex >= prices.size())
            continue;

        const double currentPrice = prices[endIndex];
        if (currentPrice == 0.0)
            continue;

        const double futurePrice = prices[futureIndex];
        const double futureReturn =
            ((futurePrice - currentPrice) / currentPrice) * 100.0;

        const double weight = match.normalizedWeight;

        weightedReturns.emplace_back(weight, futureReturn);

        if (futureReturn > 0.0)
            positiveWeight += weight;
        else if (futureReturn < 0.0)
            negativeWeight += weight;

        if (futureReturn >= minimumExpectedReturn)
            positiveReturnWeight += weight;
        else if (futureReturn <= -minimumExpectedReturn)
            negativeReturnWeight += weight;
    }

    const double totalDirectionalWeight = positiveWeight + negativeWeight;

    if (totalDirectionalWeight <= 0.0)
    {
        return result;
    }

    positiveWeight /= totalDirectionalWeight;
    negativeWeight /= totalDirectionalWeight;

    result.positiveWeight = positiveWeight;
    result.negativeWeight = negativeWeight;

    result.confidence = std::max(positiveWeight, negativeWeight);
    result.predictedPositive = positiveWeight >= negativeWeight;

    const double meaningfulTotal =
        positiveReturnWeight + negativeReturnWeight;

    if (meaningfulTotal > 0.0)
    {
        positiveReturnWeight /= meaningfulTotal;
        negativeReturnWeight /= meaningfulTotal;

        const double meaningfulConfidence =
            std::max(positiveReturnWeight, negativeReturnWeight);

        result.confidence =
            std::min(result.confidence, meaningfulConfidence);

        result.predictedPositive =
            positiveReturnWeight >= negativeReturnWeight;
    }

    /*
        NEW: dispersion penalty.

        A vote can be "6 vs 4" with historical returns that are
        wildly scattered (-8%, +12%, -3%, +9% ...) or tightly
        clustered (+1.8%, +2.1%, +1.5% ...). The vote fraction
        alone can't tell these apart, but the second case is a
        much stronger, more repeatable pattern.

        We compute the weighted standard deviation of returns
        and use it to shrink confidence when the matches disagree
        sharply on magnitude, even if they agree on direction.
    */
    if (weightedReturns.size() >= 2)
    {
        double weightedMean = 0.0;
        double weightSum = 0.0;

        for (const auto& wr : weightedReturns)
        {
            weightedMean += wr.first * wr.second;
            weightSum += wr.first;
        }

        if (weightSum > 0.0)
        {
            weightedMean /= weightSum;

            double weightedVariance = 0.0;

            for (const auto& wr : weightedReturns)
            {
                const double diff = wr.second - weightedMean;
                weightedVariance += wr.first * diff * diff;
            }

            weightedVariance /= weightSum;

            const double weightedStdDev =
                std::sqrt(weightedVariance);

            /*
                Normalize dispersion against the magnitude of the
                mean move itself. A 5% average move with 1% stddev
                is tight; a 1% average move with 5% stddev is noise.

                Add a small epsilon to avoid divide-by-zero.
            */
            const double dispersionRatio =
                weightedStdDev /
                (std::abs(weightedMean) + 1.0);

            /*
                Map dispersion ratio to a penalty multiplier in
                (0, 1]. Higher dispersion -> lower multiplier.
                The 2.0 divisor controls how aggressively spread
                is punished; tune this against your backtest.
            */
            const double dispersionPenalty =
                1.0 / (1.0 + dispersionRatio / 2.0);

            result.confidence =
    (0.7 * result.confidence) + (0.3 * result.confidence * dispersionPenalty);
        }
    }

    result.confidence =
          std::min(1.0, std::max(0.0, result.confidence));

    result.signal = result.confidence >= threshold;

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

    result.confidence5 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 5, threshold, minimumExpectedReturn);

    result.confidence10 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 10, threshold, minimumExpectedReturn);

    result.confidence15 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 15, threshold, minimumExpectedReturn);

    result.confidence30 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 30, threshold, minimumExpectedReturn);

    return result;
}