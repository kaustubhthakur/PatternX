#ifndef BACKTESTER_HPP
#define BACKTESTER_HPP

#include <cstddef>
#include <vector>

struct BacktestMetrics
{
    std::size_t samples;

    // Existing weighted prediction
    double directionalAccuracy5;
    double directionalAccuracy10;
    double directionalAccuracy15;
    double directionalAccuracy30;

    double mae5;
    double mae10;
    double mae15;
    double mae30;

    // Existing base rate
    double baseRatePositive5;
    double baseRatePositive10;
    double baseRatePositive15;
    double baseRatePositive30;

    // Existing naive baseline
    double naiveAccuracy5;
    double naiveAccuracy10;
    double naiveAccuracy15;
    double naiveAccuracy30;

    // Existing z-scores
    double zScore5;
    double zScore10;
    double zScore15;
    double zScore30;

    // NEW: majority-vote accuracy
    double majorityAccuracy5;
    double majorityAccuracy10;
    double majorityAccuracy15;
    double majorityAccuracy30;
};

BacktestMetrics runBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step
);

#endif