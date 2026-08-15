#ifndef PREDICTION_ENGINE_HPP
#define PREDICTION_ENGINE_HPP

#include <cstddef>
#include <vector>

#include "Prediction.hpp"
#include "PatternMatcher.hpp"
#include "WeightedRanking.hpp"

struct HistoricalMatchResult
{
    std::size_t windowIndex;

    double fftDistance;
    double trendDistance;
    double combinedDistance;

    double weight;
    double normalizedWeight;

    // Relative similarity represented as percentage.
    double similarityPercent;
};

struct PredictionEngineResult
{
    bool valid;

    std::size_t currentIndex;
    std::size_t windowSize;

    // Overall similarity of the current pattern
    // against the selected historical patterns.
    double patternSimilarityPercent;

    // Best historical match.
    std::size_t bestMatchIndex;
    double bestMatchSimilarityPercent;

    // Weighted future-return predictions.
    double prediction5;
    double prediction10;
    double prediction15;
    double prediction30;

    // Direction.
    bool predictionPositive5;
    bool predictionPositive10;
    bool predictionPositive15;
    bool predictionPositive30;

    // Majority voting.
    std::size_t positive5;
    std::size_t positive10;
    std::size_t positive15;
    std::size_t positive30;

    // Continuation for first 5 days.
    ContinuationPrediction continuation;

    // Selected historical matches.
    std::vector<HistoricalMatchResult> matches;
};

/*
    Predict using the most recent window.

    Example:

        predictStock(
            prices,
            30,
            10
        );

    The query window is:

        prices[prices.size() - windowSize ... prices.size() - 1]

    Historical matches are required to have their complete
    future horizon before the current query window, preventing
    look-ahead leakage.
*/
PredictionEngineResult predictStock(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK
);

/*
    Predict using an arbitrary query window.

    currentIndex is the START index of the query window.
*/
PredictionEngineResult predictAtIndex(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::size_t topK
);

#endif