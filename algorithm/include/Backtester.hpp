#ifndef BACKTESTER_HPP
#define BACKTESTER_HPP

#include <cstddef>
#include <vector>

struct BacktestMetrics
{
    std::size_t samples;

    double directionalAccuracy5;
    double directionalAccuracy10;
    double directionalAccuracy15;
    double directionalAccuracy30;

    double mae5;
    double mae10;
    double mae15;
    double mae30;

    double baseRatePositive5;
    double baseRatePositive10;
    double baseRatePositive15;
    double baseRatePositive30;

    double naiveAccuracy5;
    double naiveAccuracy10;
    double naiveAccuracy15;
    double naiveAccuracy30;

    double zScore5;
    double zScore10;
    double zScore15;
    double zScore30;



    double confidenceThreshold;

    std::size_t signals5;
    std::size_t signals10;
    std::size_t signals15;
    std::size_t signals30;

    double coverage5;
    double coverage10;
    double coverage15;
    double coverage30;

    double signalAccuracy5;
    double signalAccuracy10;
    double signalAccuracy15;
    double signalAccuracy30;

    double averageReturnWhenSignaled5;
    double averageReturnWhenSignaled10;
    double averageReturnWhenSignaled15;
    double averageReturnWhenSignaled30;
};


BacktestMetrics runBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step
);



BacktestMetrics runConfidenceBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step,
    double trainRatio,
    double confidenceThreshold
);

#endif