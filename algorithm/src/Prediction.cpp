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


// ------------------------------------------------------------
// NEW: Day-by-day continuation (days 1 through 5)
//
// Gives the cumulative % return at EVERY day from 1 to 5,
// not just the fixed +5 checkpoint. Lets the caller show a
// short-term trajectory instead of only an endpoint number.
//
// Kept as a separate struct/function pair so nothing about
// the existing FutureReturns / PredictionResult contract
// changes.
// ------------------------------------------------------------
DailyContinuation calculateDailyContinuation(
    const std::vector<double>& prices,
    std::size_t windowIndex,
    std::size_t windowSize
)
{
    DailyContinuation result{
        0.0, 0.0, 0.0, 0.0, 0.0
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

    double* const dayFields[5] = {
        &result.day1,
        &result.day2,
        &result.day3,
        &result.day4,
        &result.day5
    };

    for (std::size_t day = 1; day <= 5; ++day)
    {
        const std::size_t futureIndex =
            endIndex + day;

        if (futureIndex >= prices.size())
        {
            continue;
        }

        *dayFields[day - 1] =
            ((prices[futureIndex] - currentPrice)
             / currentPrice) * 100.0;
    }

    return result;
}


ContinuationPrediction calculateWeightedContinuation(
    const std::vector<DailyContinuation>& continuations,
    const std::vector<double>& normalizedWeights
)
{
    ContinuationPrediction result{
        0.0, 0.0, 0.0, 0.0, 0.0
    };

    if (continuations.empty() ||
        normalizedWeights.empty())
    {
        return result;
    }

    if (continuations.size() !=
        normalizedWeights.size())
    {
        return result;
    }

    for (std::size_t i = 0;
         i < continuations.size();
         ++i)
    {
        const double weight =
            normalizedWeights[i];

        result.day1 += weight * continuations[i].day1;
        result.day2 += weight * continuations[i].day2;
        result.day3 += weight * continuations[i].day3;
        result.day4 += weight * continuations[i].day4;
        result.day5 += weight * continuations[i].day5;
    }

    return result;
}