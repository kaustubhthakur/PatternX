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

 
    double similarityPercent;
};

struct PredictionEngineResult
{
    bool valid;

    std::size_t currentIndex;
    std::size_t windowSize;


    double patternSimilarityPercent;


    std::size_t bestMatchIndex;
    double bestMatchSimilarityPercent;

 
    double prediction5;
    double prediction10;
    double prediction15;
    double prediction30;


    bool predictionPositive5;
    bool predictionPositive10;
    bool predictionPositive15;
    bool predictionPositive30;

    
    std::size_t positive5;
    std::size_t positive10;
    std::size_t positive15;
    std::size_t positive30;


    ContinuationPrediction continuation;

 
    std::vector<HistoricalMatchResult> matches;
};


PredictionEngineResult predictStock(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK
);

PredictionEngineResult predictAtIndex(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::size_t topK
);

#endif