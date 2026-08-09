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
};

BacktestMetrics runBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step
);

#endif