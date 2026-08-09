#ifndef PREDICTION_HPP
#define PREDICTION_HPP

#include <cstddef>
#include <vector>

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

FutureReturns calculateFutureReturns(
    const std::vector<double>& prices,
    std::size_t windowIndex,
    std::size_t windowSize
);

PredictionResult calculateWeightedPrediction(
    const std::vector<FutureReturns>& futureReturns,
    const std::vector<double>& normalizedWeights
);

#endif