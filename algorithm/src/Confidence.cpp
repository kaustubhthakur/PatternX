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
    double minimumExpectedReturn,
    double horizonTypicalStdDev
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
        Dispersion penalty, normalized against what's TYPICAL for
        THIS SPECIFIC HORIZON (horizonTypicalStdDev), instead of a
        flat constant. Longer horizons naturally have wider return
        dispersion regardless of pattern quality, so without this
        normalization, +30 day predictions get punished far more
        than +5 day ones purely due to horizon length, not because
        the pattern match is actually worse.
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
                Normalize against the horizon's typical stddev
                instead of a flat "+1.0". Fall back to 1.0 if the
                typical stddev couldn't be computed (e.g. not
                enough history), to avoid divide-by-zero.
            */
            const double normalizer =
                (horizonTypicalStdDev > 1e-6)
                    ? horizonTypicalStdDev
                    : 1.0;

            const double dispersionRatio =
                weightedStdDev / normalizer;

            /*
                dispersionRatio ~1.0  -> "as spread out as normal
                                          for this horizon" -> mild
                                          penalty.
                dispersionRatio << 1.0 -> unusually tight cluster
                                          -> little to no penalty.
                dispersionRatio >> 1.0 -> unusually scattered
                                          -> strong penalty.
            */
            const double dispersionPenalty =
                1.0 / (1.0 + dispersionRatio / 2.0);

            result.confidence =
                (0.7 * result.confidence) +
                (0.3 * result.confidence * dispersionPenalty);
        }
    }

    result.confidence =
        std::min(1.0, std::max(0.0, result.confidence));

    result.signal = result.confidence >= threshold;

    return result;
}

} // namespace

double calculateHorizonTypicalStdDev(
    const std::vector<double>& prices,
    std::size_t horizon,
    std::size_t cutoffIndex
)
{
    if (cutoffIndex == 0 || cutoffIndex > prices.size())
    {
        cutoffIndex = prices.size();
    }

    std::vector<double> returns;
    returns.reserve(cutoffIndex);

    for (std::size_t i = 0; i + horizon < cutoffIndex; ++i)
    {
        const double currentPrice = prices[i];

        if (currentPrice == 0.0)
            continue;

        const double futurePrice = prices[i + horizon];

        const double r =
            ((futurePrice - currentPrice) / currentPrice) * 100.0;

        returns.push_back(r);
    }

    if (returns.size() < 2)
    {
        return 0.0;
    }

    double mean = 0.0;
    for (double r : returns)
        mean += r;
    mean /= static_cast<double>(returns.size());

    double variance = 0.0;
    for (double r : returns)
    {
        const double diff = r - mean;
        variance += diff * diff;
    }
    variance /= static_cast<double>(returns.size());

    return std::sqrt(variance);
}

ConfidenceResult calculateConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    double threshold,
    double minimumExpectedReturn
)
{
    ConfidenceResult result{};

    const double typicalStdDev5 =
        calculateHorizonTypicalStdDev(prices, 5, prices.size());

    const double typicalStdDev10 =
        calculateHorizonTypicalStdDev(prices, 10, prices.size());

    const double typicalStdDev15 =
        calculateHorizonTypicalStdDev(prices, 15, prices.size());

    const double typicalStdDev30 =
        calculateHorizonTypicalStdDev(prices, 30, prices.size());

    result.confidence5 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 5,
        threshold, minimumExpectedReturn, typicalStdDev5);

    result.confidence10 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 10,
        threshold, minimumExpectedReturn, typicalStdDev10);

    result.confidence15 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 15,
        threshold, minimumExpectedReturn, typicalStdDev15);

    result.confidence30 = calculateHorizonConfidence(
        prices, weightedMatches, windowSize, 30,
        threshold, minimumExpectedReturn, typicalStdDev30);

    return result;
}