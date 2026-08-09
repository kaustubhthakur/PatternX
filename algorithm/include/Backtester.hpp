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

    // --------------------------------------------------------
    // Validation additions
    // --------------------------------------------------------

    // % of actual returns that were positive, per horizon.
    // If this is far from 50%, a model can score well above
    // 50% directional accuracy just by always guessing the
    // majority sign -- without reading any real pattern.
    double baseRatePositive5;
    double baseRatePositive10;
    double baseRatePositive15;
    double baseRatePositive30;

    // Directional accuracy of a trivial predictor: guess the
    // same sign as the stock's trailing 5-day return. If this
    // matches or beats the FFT model's accuracy, the FFT
    // machinery isn't adding value over a much simpler rule.
    double naiveAccuracy5;
    double naiveAccuracy10;
    double naiveAccuracy15;
    double naiveAccuracy30;

    // Z-score for the model's directional accuracy against a
    // 50% (random) null hypothesis, using the normal
    // approximation to the binomial distribution:
    //
    //   z = (observed - 0.5) / sqrt(0.25 / n)
    //
    // |z| > 1.96 is roughly the conventional threshold for
    // "statistically significant at the 95% level" -- treat
    // this as a rough guide, not a rigorous test, since
    // backtest samples here are overlapping/correlated rather
    // than independent, which inflates apparent significance.
    double zScore5;
    double zScore10;
    double zScore15;
    double zScore30;
};

BacktestMetrics runBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step
);

#endif