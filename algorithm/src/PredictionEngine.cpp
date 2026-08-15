#include "../include/PredictionEngine.hpp"

#include "../include/FFT.hpp"
#include "../include/Normalizer.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <vector>

namespace
{

constexpr double EPSILON = 1e-12;


double clampValue(
    double value,
    double low,
    double high
)
{
    if (value < low)
    {
        return low;
    }

    if (value > high)
    {
        return high;
    }

    return value;
}


double calculateSimilarity(
    double distance,
    double bestDistance
)
{
    if (!std::isfinite(distance))
    {
        return 0.0;
    }

    if (!std::isfinite(bestDistance))
    {
        return 0.0;
    }

    if (distance < EPSILON)
    {
        return 100.0;
    }

  
    const double ratio =
        bestDistance / distance;

    const double similarity =
        ratio * 100.0;

    return clampValue(
        similarity,
        0.0,
        100.0
    );
}



bool buildHistoricalData(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::vector<std::vector<double>>& signatures,
    std::vector<std::vector<double>>& windows,
    std::vector<std::size_t>& candidateIndices
)
{
    signatures.clear();
    windows.clear();
    candidateIndices.clear();

    if (windowSize == 0)
    {
        return false;
    }

    if (currentIndex + windowSize >
        prices.size())
    {
        return false;
    }



    if (currentIndex <= windowSize)
    {
        return false;
    }

    const std::size_t lastHistoricalStart =
        currentIndex >
            (windowSize + 30)
            ? currentIndex -
              windowSize -
              30
            : 0;

    if (lastHistoricalStart == 0 &&
        currentIndex <= windowSize + 30)
    {
        return false;
    }

    for (std::size_t start = 0;
         start <= lastHistoricalStart;
         ++start)
    {
        const std::size_t end =
            start + windowSize - 1;

        if (end >= currentIndex)
        {
            break;
        }

        if (end + 30 >= currentIndex)
        {
            continue;
        }

        if (end >= prices.size())
        {
            continue;
        }

        std::vector<double> window(
            prices.begin() + start,
            prices.begin() +
                start +
                windowSize
        );

        std::vector<double> normalized =
            normalizeWindow(window);

        std::vector<std::complex<double>> fft =
            computeFFT(normalized);

        std::vector<double> signature =
            computeMagnitude(fft);

        signatures.push_back(
            std::move(signature)
        );

        windows.push_back(
            std::move(window)
        );

        candidateIndices.push_back(
            start
        );
    }

    return !candidateIndices.empty();
}


bool buildCurrentWindow(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::vector<double>& window,
    std::vector<double>& signature
)
{
    if (windowSize == 0)
    {
        return false;
    }

    if (currentIndex + windowSize >
        prices.size())
    {
        return false;
    }

    window.assign(
        prices.begin() + currentIndex,
        prices.begin() +
            currentIndex +
            windowSize
    );

    const std::vector<double> normalized =
        normalizeWindow(window);

    const std::vector<std::complex<double>> fft =
        computeFFT(normalized);

    signature =
        computeMagnitude(fft);

    return !signature.empty();
}


HistoricalMatchResult convertMatch(
    const WeightedMatch& weightedMatch,
    const std::vector<PatternMatch>& matches,
    double bestDistance
)
{
    HistoricalMatchResult result{};

    result.windowIndex =
        weightedMatch.windowIndex;

 
    for (const PatternMatch& match :
         matches)
    {
        if (match.windowIndex ==
            weightedMatch.windowIndex)
        {
            result.fftDistance =
                match.fftDistance;

            result.trendDistance =
                match.trendDistance;

            result.combinedDistance =
                match.combinedDistance;

            break;
        }
    }

    result.weight =
        weightedMatch.weight;

    result.normalizedWeight =
        weightedMatch.normalizedWeight;

    result.similarityPercent =
        calculateSimilarity(
            result.combinedDistance,
            bestDistance
        );

    return result;
}



double calculateWeightedSimilarity(
    const std::vector<HistoricalMatchResult>& matches
)
{
    if (matches.empty())
    {
        return 0.0;
    }

    double result = 0.0;

    for (const auto& match :
         matches)
    {
        result +=
            match.normalizedWeight *
            match.similarityPercent;
    }

    return clampValue(
        result,
        0.0,
        100.0
    );
}

} 


PredictionEngineResult predictAtIndex(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::size_t topK
)
{
    PredictionEngineResult result{};

    result.valid = false;
    result.currentIndex = currentIndex;
    result.windowSize = windowSize;

    if (prices.empty())
    {
        return result;
    }

    if (windowSize == 0)
    {
        return result;
    }

    if (topK == 0)
    {
        return result;
    }

    if (currentIndex +
            windowSize +
            30 >
        prices.size())
    {
        return result;
    }

    std::vector<double> currentWindow;
    std::vector<double> currentSignature;

    if (!buildCurrentWindow(
            prices,
            currentIndex,
            windowSize,
            currentWindow,
            currentSignature))
    {
        return result;
    }

    std::vector<std::vector<double>>
        historicalSignatures;

    std::vector<std::vector<double>>
        historicalWindows;

    std::vector<std::size_t>
        candidateIndices;

    if (!buildHistoricalData(
            prices,
            currentIndex,
            windowSize,
            historicalSignatures,
            historicalWindows,
            candidateIndices))
    {
        return result;
    }


    const std::vector<PatternMatch> rawMatches =
        findTopMatches(
            currentSignature,
            historicalSignatures,
            currentWindow,
            historicalWindows,
            currentIndex,
            topK,
            windowSize
        );

    if (rawMatches.empty())
    {
        return result;
    }

    std::vector<std::size_t> actualIndices;
    std::vector<double> distances;

    actualIndices.reserve(
        rawMatches.size()
    );

    distances.reserve(
        rawMatches.size()
    );

    for (const PatternMatch& match :
         rawMatches)
    {
        if (match.windowIndex >=
            candidateIndices.size())
        {
            continue;
        }

        actualIndices.push_back(
            candidateIndices[
                match.windowIndex
            ]
        );

        distances.push_back(
            match.combinedDistance
        );
    }

    if (actualIndices.empty())
    {
        return result;
    }

 
    const std::vector<WeightedMatch>
        weightedMatches =
            calculateWeights(
                actualIndices,
                distances
            );

    if (weightedMatches.empty())
    {
        return result;
    }

 
    double bestDistance =
        std::numeric_limits<double>::max();

    for (const auto& match :
         rawMatches)
    {
        bestDistance =
            std::min(
                bestDistance,
                match.combinedDistance
            );
    }

    for (const WeightedMatch& weightedMatch :
         weightedMatches)
    {
        HistoricalMatchResult matchResult =
            convertMatch(
                weightedMatch,
                rawMatches,
                bestDistance
            );

        result.matches.push_back(
            matchResult
        );
    }

    if (result.matches.empty())
    {
        return result;
    }

  
    std::sort(
        result.matches.begin(),
        result.matches.end(),
        [](
            const HistoricalMatchResult& a,
            const HistoricalMatchResult& b
        )
        {
            return a.normalizedWeight >
                   b.normalizedWeight;
        }
    );

 
    result.bestMatchIndex =
        result.matches.front().windowIndex;

    result.bestMatchSimilarityPercent =
        result.matches.front()
            .similarityPercent;

    result.patternSimilarityPercent =
        calculateWeightedSimilarity(
            result.matches
        );



    std::vector<FutureReturns>
        futureReturns;

    std::vector<double>
        normalizedWeights;

    futureReturns.reserve(
        weightedMatches.size()
    );

    normalizedWeights.reserve(
        weightedMatches.size()
    );

    for (const WeightedMatch& match :
         weightedMatches)
    {
        futureReturns.push_back(
            calculateFutureReturns(
                prices,
                match.windowIndex,
                windowSize
            )
        );

        normalizedWeights.push_back(
            match.normalizedWeight
        );
    }

    const PredictionResult prediction =
        calculateWeightedPrediction(
            futureReturns,
            normalizedWeights
        );

    result.prediction5 =
        prediction.prediction5;

    result.prediction10 =
        prediction.prediction10;

    result.prediction15 =
        prediction.prediction15;

    result.prediction30 =
        prediction.prediction30;

    result.predictionPositive5 =
        result.prediction5 > 0.0;

    result.predictionPositive10 =
        result.prediction10 > 0.0;

    result.predictionPositive15 =
        result.prediction15 > 0.0;

    result.predictionPositive30 =
        result.prediction30 > 0.0;


    const MajorityVoteResult majority =
        calculateMajorityVote(
            futureReturns
        );

    result.positive5 =
        majority.positive5;

    result.positive10 =
        majority.positive10;

    result.positive15 =
        majority.positive15;

    result.positive30 =
        majority.positive30;


    std::vector<DailyContinuation>
        continuations;

    continuations.reserve(
        weightedMatches.size()
    );

    for (const WeightedMatch& match :
         weightedMatches)
    {
        continuations.push_back(
            calculateDailyContinuation(
                prices,
                match.windowIndex,
                windowSize
            )
        );
    }

    result.continuation =
        calculateWeightedContinuation(
            continuations,
            normalizedWeights
        );

    result.valid = true;

    return result;
}


PredictionEngineResult predictStock(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK
)
{
    PredictionEngineResult result{};

    if (prices.size() <=
        windowSize + 30)
    {
        return result;
    }
 
    const std::size_t currentIndex =
        prices.size() -
        windowSize;

    return predictAtIndex(
        prices,
        currentIndex,
        windowSize,
        topK
    );
}