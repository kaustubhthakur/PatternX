#ifndef PREDICTION_HPP
#define PREDICTION_HPP

#include <cstddef>
#include <vector>

struct DailyContinuation
{
    double day1;
    double day2;
    double day3;
    double day4;
    double day5;
};

struct ContinuationPrediction
{
    double day1;
    double day2;
    double day3;
    double day4;
    double day5;
};

DailyContinuation calculateDailyContinuation(
    const std::vector<double>& prices,
    std::size_t windowIndex,
    std::size_t windowSize
);

ContinuationPrediction calculateWeightedContinuation(
    const std::vector<DailyContinuation>& continuations,
    const std::vector<double>& normalizedWeights
);
struct FutureReturns
{
    double return5;
    double return10;
    double return15;
    double return30;
};

struct PredictionResult
{
    double prediction5;
    double prediction10;
    double prediction15;
    double prediction30;
};

// Majority vote result
struct MajorityVoteResult
{
    bool prediction5;
    bool prediction10;
    bool prediction15;
    bool prediction30;

    std::size_t positive5;
    std::size_t positive10;
    std::size_t positive15;
    std::size_t positive30;
};

FutureReturns calculateFutureReturns(
    const std::vector<double>& prices,
    std::size_t windowIndex,
    std::size_t windowSize
);

PredictionResult calculateWeightedPrediction(
    const std::vector<FutureReturns>& futureReturns,
    const std::vector<double>& normalizedWeights
);

MajorityVoteResult calculateMajorityVote(
    const std::vector<FutureReturns>& futureReturns
);

#endif