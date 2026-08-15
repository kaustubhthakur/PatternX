#ifndef MULTI_WINDOW_PREDICTION_HPP
#define MULTI_WINDOW_PREDICTION_HPP

#include <array>
#include <cstddef>
#include <vector>

struct MultiWindowPrediction
{
    double prediction5 = 0.0;
    double prediction10 = 0.0;
    double prediction15 = 0.0;
    double prediction30 = 0.0;

    std::array<double, 5> continuationPath{};   // day 1..5 cumulative % return, ensemble-averaged

    std::size_t validWindowModels = 0;
};

MultiWindowPrediction calculateMultiWindowPrediction(
    const std::vector<double>& prices,
    std::size_t anchorEndIndex,
    std::size_t topK
);

#endif