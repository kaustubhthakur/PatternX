#ifndef BACKTESTER_HPP
#define BACKTESTER_HPP

#include <cstddef>
#include <vector>

struct BacktestMetrics
{
    std::size_t samples = 0;

    double directionalAccuracy5 = 0.0;
    double directionalAccuracy10 = 0.0;
    double directionalAccuracy15 = 0.0;
    double directionalAccuracy30 = 0.0;

    double mae5 = 0.0;
    double mae10 = 0.0;
    double mae15 = 0.0;
    double mae30 = 0.0;

    double baseRatePositive5 = 0.0;
    double baseRatePositive10 = 0.0;
    double baseRatePositive15 = 0.0;
    double baseRatePositive30 = 0.0;

    double naiveAccuracy5 = 0.0;
    double naiveAccuracy10 = 0.0;
    double naiveAccuracy15 = 0.0;
    double naiveAccuracy30 = 0.0;

    double zScore5 = 0.0;
    double zScore10 = 0.0;
    double zScore15 = 0.0;
    double zScore30 = 0.0;

    double confidenceThreshold = 0.0;

    std::size_t signals5 = 0;
    std::size_t signals10 = 0;
    std::size_t signals15 = 0;
    std::size_t signals30 = 0;

    double coverage5 = 0.0;
    double coverage10 = 0.0;
    double coverage15 = 0.0;
    double coverage30 = 0.0;

    double signalAccuracy5 = 0.0;
    double signalAccuracy10 = 0.0;
    double signalAccuracy15 = 0.0;
    double signalAccuracy30 = 0.0;

    double averageReturnWhenSignaled5 = 0.0;
    double averageReturnWhenSignaled10 = 0.0;
    double averageReturnWhenSignaled15 = 0.0;
    double averageReturnWhenSignaled30 = 0.0;

    double totalReturn5 = 0.0;
    double totalReturn10 = 0.0;
    double totalReturn15 = 0.0;
    double totalReturn30 = 0.0;

    double averageTradeReturn5 = 0.0;
    double averageTradeReturn10 = 0.0;
    double averageTradeReturn15 = 0.0;
    double averageTradeReturn30 = 0.0;

    double winRate5 = 0.0;
    double winRate10 = 0.0;
    double winRate15 = 0.0;
    double winRate30 = 0.0;

    double profitFactor5 = 0.0;
    double profitFactor10 = 0.0;
    double profitFactor15 = 0.0;
    double profitFactor30 = 0.0;

    double maxDrawdown5 = 0.0;
    double maxDrawdown10 = 0.0;
    double maxDrawdown15 = 0.0;
    double maxDrawdown30 = 0.0;
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