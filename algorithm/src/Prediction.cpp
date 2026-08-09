#include "../include/Prediction.hpp"

FutureReturns calculateFutureReturns(
    const std::vector<double>& prices,
    std::size_t windowIndex,
    std::size_t windowSize
)
{
    FutureReturns result{
        0.0,
        0.0,
        0.0,
        0.0
    };

    const std::size_t endIndex =
        windowIndex + windowSize - 1;

    if (endIndex >= prices.size())
    {
        return result;
    }

    const double currentPrice =
        prices[endIndex];

    if (currentPrice == 0.0)
    {
        return result;
    }

    if (endIndex + 5 < prices.size())
    {
        result.return5 =
            ((prices[endIndex + 5] - currentPrice)
             / currentPrice) * 100.0;
    }

    if (endIndex + 10 < prices.size())
    {
        result.return10 =
            ((prices[endIndex + 10] - currentPrice)
             / currentPrice) * 100.0;
    }

    if (endIndex + 15 < prices.size())
    {
        result.return15 =
            ((prices[endIndex + 15] - currentPrice)
             / currentPrice) * 100.0;
    }

    if (endIndex + 30 < prices.size())
    {
        result.return30 =
            ((prices[endIndex + 30] - currentPrice)
             / currentPrice) * 100.0;
    }

    return result;
}


PredictionResult calculateWeightedPrediction(
    const std::vector<FutureReturns>& futureReturns,
    const std::vector<double>& normalizedWeights
)
{
    PredictionResult result{
        0.0,
        0.0,
        0.0,
        0.0
    };

    if (futureReturns.empty() ||
        normalizedWeights.empty())
    {
        return result;
    }

    if (futureReturns.size() !=
        normalizedWeights.size())
    {
        return result;
    }

    for (std::size_t i = 0;
         i < futureReturns.size();
         ++i)
    {
        double weight =
            normalizedWeights[i];

        result.prediction5 +=
            weight * futureReturns[i].return5;

        result.prediction10 +=
            weight * futureReturns[i].return10;

        result.prediction15 +=
            weight * futureReturns[i].return15;

        result.prediction30 +=
            weight * futureReturns[i].return30;
    }

    return result;
}


// ------------------------------------------------------------
// Majority vote prediction
//
// For each horizon, count how many historical matches
// produced a positive return.
//
// More positive matches  -> UP
// More negative matches  -> DOWN
//
// Existing weighted prediction remains completely unchanged.
// ------------------------------------------------------------
MajorityVoteResult calculateMajorityVote(
    const std::vector<FutureReturns>& futureReturns
)
{
    MajorityVoteResult result{
        false,
        false,
        false,
        false,
        0,
        0,
        0,
        0
    };

    if (futureReturns.empty())
    {
        return result;
    }

    for (const auto& returns : futureReturns)
    {
        if (returns.return5 > 0.0)
        {
            result.positive5++;
        }

        if (returns.return10 > 0.0)
        {
            result.positive10++;
        }

        if (returns.return15 > 0.0)
        {
            result.positive15++;
        }

        if (returns.return30 > 0.0)
        {
            result.positive30++;
        }
    }

    const std::size_t total =
        futureReturns.size();

    result.prediction5 =
        result.positive5 > total / 2;

    result.prediction10 =
        result.positive10 > total / 2;

    result.prediction15 =
        result.positive15 > total / 2;

    result.prediction30 =
        result.positive30 > total / 2;

    return result;
}