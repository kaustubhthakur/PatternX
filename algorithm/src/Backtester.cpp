#include "../include/Backtester.hpp"

#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"
#include "../include/PatternMatcher.hpp"
#include "../include/WeightedRanking.hpp"
#include "../include/Confidence.hpp"
#include "../include/Calibration.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <vector>

namespace
{

struct HorizonState
{
    std::size_t signals = 0;
    std::size_t correctSignals = 0;
    std::size_t validSamples = 0;
    std::size_t actualPositive = 0;
    std::size_t naiveCorrect = 0;
    std::size_t majorityCorrect = 0;

    double error = 0.0;
    double actualReturnSum = 0.0;
    double totalPnL = 0.0;
    double grossProfit = 0.0;
    double grossLoss = 0.0;

    double equity = 0.0;
    double peakEquity = 0.0;
    double maxDrawdown = 0.0;
};

struct MajorityCounts
{
    std::size_t positive = 0;
    std::size_t negative = 0;
};

double calculateReturn(
    const std::vector<double>& prices,
    std::size_t startIndex,
    std::size_t horizon
)
{
    const std::size_t futureIndex =
        startIndex + horizon;

    if (futureIndex >= prices.size())
    {
        return 0.0;
    }

    const double currentPrice =
        prices[startIndex];

    if (currentPrice == 0.0)
    {
        return 0.0;
    }

    return (
        (prices[futureIndex] - currentPrice)
        / currentPrice
    ) * 100.0;
}

double calculateTrailingReturn(
    const std::vector<double>& prices,
    std::size_t index,
    std::size_t days
)
{
    if (index < days)
    {
        return 0.0;
    }

    const double previousPrice =
        prices[index - days];

    if (previousPrice == 0.0)
    {
        return 0.0;
    }

    return (
        (prices[index] - previousPrice)
        / previousPrice
    ) * 100.0;
}

double calculateDirectionalTradeReturn(
    const std::vector<double>& prices,
    std::size_t index,
    std::size_t horizon,
    bool predictedPositive
)
{
    const double rawReturn =
        calculateReturn(
            prices,
            index,
            horizon
        );

    return predictedPositive
        ? rawReturn
        : -rawReturn;
}

double calculateZScore(
    double accuracyPercent,
    std::size_t samples
)
{
    if (samples == 0)
    {
        return 0.0;
    }

    const double observed =
        accuracyPercent / 100.0;

    const double standardError =
        std::sqrt(
            0.25 /
            static_cast<double>(samples)
        );

    if (standardError == 0.0)
    {
        return 0.0;
    }

    return (
        observed - 0.5
    ) / standardError;
}

bool isCorrectDirection(
    double prediction,
    double actual
)
{
    if (prediction == 0.0 || actual == 0.0)
    {
        return false;
    }

    return (
        (prediction > 0.0 && actual > 0.0) ||
        (prediction < 0.0 && actual < 0.0)
    );
}

bool buildMatches(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t horizon,
    std::vector<WeightedMatch>& weightedMatches
)
{
    weightedMatches.clear();

    if (windowSize == 0 ||
        currentIndex + windowSize > prices.size())
    {
        return false;
    }

    std::vector<std::vector<double>> signatures;
    std::vector<std::vector<double>> windows;
    std::vector<std::size_t> candidateIndices;

    signatures.reserve(currentIndex + 1);
    windows.reserve(currentIndex + 1);

    for (std::size_t start = 0;
         start <= currentIndex;
         ++start)
    {
        if (start + windowSize > prices.size())
        {
            break;
        }

        std::vector<double> window(
            prices.begin() + start,
            prices.begin() + start + windowSize
        );

        std::vector<double> normalized =
            normalizeWindow(window);

        std::vector<std::complex<double>> fftResult =
            computeFFT(normalized);

        signatures.push_back(
            computeMagnitude(fftResult)
        );

        windows.push_back(
            std::move(window)
        );
    }

    if (currentIndex >= signatures.size())
    {
        return false;
    }

    for (std::size_t start = 0;
         start < currentIndex;
         ++start)
    {
        const std::size_t historicalEnd =
            start + windowSize - 1;

        const std::size_t futureIndex =
            historicalEnd + horizon;

        if (futureIndex >= currentIndex)
        {
            continue;
        }

        candidateIndices.push_back(start);
    }

    if (candidateIndices.empty())
    {
        return false;
    }

    std::vector<std::vector<double>> historicalSignatures;
    std::vector<std::vector<double>> historicalWindows;

    historicalSignatures.reserve(
        candidateIndices.size()
    );

    historicalWindows.reserve(
        candidateIndices.size()
    );

    for (const std::size_t index : candidateIndices)
    {
        historicalSignatures.push_back(
            signatures[index]
        );

        historicalWindows.push_back(
            windows[index]
        );
    }

    const std::vector<double>& currentSignature =
        signatures[currentIndex];

    const std::vector<double>& currentWindow =
        windows[currentIndex];

    std::vector<PatternMatch> matches =
        findTopMatches(
            currentSignature,
            historicalSignatures,
            currentWindow,
            historicalWindows,
            currentIndex,
            topK,
            windowSize
        );

    if (matches.empty())
    {
        return false;
    }

    std::vector<std::size_t> windowIndices;
    std::vector<double> distances;

    windowIndices.reserve(matches.size());
    distances.reserve(matches.size());

    for (const auto& match : matches)
    {
        if (match.windowIndex >= candidateIndices.size())
        {
            continue;
        }

        windowIndices.push_back(
            candidateIndices[
                match.windowIndex
            ]
        );

        distances.push_back(
            match.combinedDistance
        );
    }

    if (windowIndices.empty())
    {
        return false;
    }

    weightedMatches =
        calculateWeights(
            windowIndices,
            distances
        );

    return !weightedMatches.empty();
}

double calculateWeightedPrediction(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& matches,
    std::size_t windowSize,
    std::size_t horizon
)
{
    double prediction = 0.0;

    for (const auto& match : matches)
    {
        const std::size_t historicalEnd =
            match.windowIndex +
            windowSize - 1;

        const double historicalReturn =
            calculateReturn(
                prices,
                historicalEnd,
                horizon
            );

        prediction +=
            match.normalizedWeight *
            historicalReturn;
    }

    return prediction;
}

double calculatePredictionAgreement(
    bool valid5,
    bool positive5,
    bool valid10,
    bool positive10,
    bool valid15,
    bool positive15,
    bool valid30,
    bool positive30
)
{
    std::size_t valid = 0;
    std::size_t positive = 0;

    if (valid5)
    {
        ++valid;

        if (positive5)
        {
            ++positive;
        }
    }

    if (valid10)
    {
        ++valid;

        if (positive10)
        {
            ++positive;
        }
    }

    if (valid15)
    {
        ++valid;

        if (positive15)
        {
            ++positive;
        }
    }

    if (valid30)
    {
        ++valid;

        if (positive30)
        {
            ++positive;
        }
    }

    if (valid == 0)
    {
        return 0.0;
    }

    const std::size_t negative =
        valid - positive;

    const std::size_t dominant =
        std::max(
            positive,
            negative
        );

    return static_cast<double>(dominant) /
           static_cast<double>(valid);
}

std::vector<CalibrationPoint> collectCalibrationPoints(
    const std::vector<double>& prices,
    std::size_t trainingEnd,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t horizon,
    std::size_t step
)
{
    std::vector<CalibrationPoint> points;

    if (step == 0)
    {
        step = 1;
    }

    if (trainingEnd <= windowSize)
    {
        return points;
    }

    const std::size_t minimumQuery =
        windowSize + horizon;

    for (std::size_t queryIndex = minimumQuery;
         queryIndex < trainingEnd;
         queryIndex += step)
    {
        const std::size_t predictionIndex =
            queryIndex + windowSize - 1;

        if (predictionIndex >= trainingEnd)
        {
            break;
        }

        if (predictionIndex + horizon >= trainingEnd)
        {
            break;
        }

        std::vector<WeightedMatch> matches;

        if (!buildMatches(
                prices,
                queryIndex,
                windowSize,
                topK,
                horizon,
                matches))
        {
            continue;
        }

        const ConfidenceResult confidence =
            calculateConfidence(
                prices,
                matches,
                windowSize,
                0.0
            );

        double rawConfidence = 0.0;
        bool predictedPositive = false;

        if (horizon == 5)
        {
            rawConfidence =
                confidence.confidence5.confidence;

            predictedPositive =
                confidence.confidence5.predictedPositive;
        }
        else if (horizon == 10)
        {
            rawConfidence =
                confidence.confidence10.confidence;

            predictedPositive =
                confidence.confidence10.predictedPositive;
        }
        else if (horizon == 15)
        {
            rawConfidence =
                confidence.confidence15.confidence;

            predictedPositive =
                confidence.confidence15.predictedPositive;
        }
        else if (horizon == 30)
        {
            rawConfidence =
                confidence.confidence30.confidence;

            predictedPositive =
                confidence.confidence30.predictedPositive;
        }
        else
        {
            continue;
        }

        const double actualReturn =
            calculateReturn(
                prices,
                predictionIndex,
                horizon
            );

        if (actualReturn == 0.0)
        {
            continue;
        }

        const bool correct =
            predictedPositive
                ? actualReturn > 0.0
                : actualReturn < 0.0;

        points.push_back(
            CalibrationPoint{
                rawConfidence,
                correct
            }
        );
    }

    return points;
}

std::vector<CalibrationBucket> buildTrainingCalibrationTable(
    const std::vector<double>& prices,
    std::size_t trainingEnd,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t horizon,
    std::size_t step
)
{
    const auto points =
        collectCalibrationPoints(
            prices,
            trainingEnd,
            windowSize,
            topK,
            horizon,
            step
        );

    return buildCalibrationTable(
        points,
        10
    );
}

MajorityCounts calculateMajorityCounts(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::size_t horizon
)
{
    MajorityCounts counts{};

    if (currentIndex <= windowSize)
    {
        return counts;
    }

    for (std::size_t start = 0;
         start < currentIndex;
         ++start)
    {
        const std::size_t historicalEnd =
            start + windowSize - 1;

        if (historicalEnd >= currentIndex)
        {
            continue;
        }

        const std::size_t futureIndex =
            historicalEnd + horizon;

        if (futureIndex >= currentIndex)
        {
            continue;
        }

        const double result =
            calculateReturn(
                prices,
                historicalEnd,
                horizon
            );

        if (result > 0.0)
        {
            ++counts.positive;
        }
        else if (result < 0.0)
        {
            ++counts.negative;
        }
    }

    return counts;
}

bool majorityPrediction(
    const MajorityCounts& counts
)
{
    if (counts.positive == 0 &&
        counts.negative == 0)
    {
        return false;
    }

    return counts.positive >=
           counts.negative;
}

void updateSignalStatistics(
    HorizonState& state,
    double actualReturn,
    double tradeReturn
)
{
    ++state.signals;

    state.actualReturnSum +=
        actualReturn;

    state.totalPnL +=
        tradeReturn;

    if (tradeReturn > 0.0)
    {
        ++state.correctSignals;
        state.grossProfit += tradeReturn;
    }
    else if (tradeReturn < 0.0)
    {
        state.grossLoss +=
            std::abs(tradeReturn);
    }

    state.equity += tradeReturn;

    state.peakEquity =
        std::max(
            state.peakEquity,
            state.equity
        );

    state.maxDrawdown =
        std::max(
            state.maxDrawdown,
            state.peakEquity -
            state.equity
        );
}

void updateBaselineStatistics(
    HorizonState& state,
    double actualReturn,
    bool naivePrediction,
    bool majorityPredictionValue
)
{
    ++state.validSamples;

    if (actualReturn > 0.0)
    {
        ++state.actualPositive;
    }

    if (
        (naivePrediction && actualReturn > 0.0) ||
        (!naivePrediction && actualReturn < 0.0)
    )
    {
        ++state.naiveCorrect;
    }

    if (
        (majorityPredictionValue &&
         actualReturn > 0.0) ||
        (!majorityPredictionValue &&
         actualReturn < 0.0)
    )
    {
        ++state.majorityCorrect;
    }
}

void assignMetrics(
    BacktestMetrics& metrics,
    const HorizonState& state,
    double& mae,
    double& coverage,
    double& signalAccuracy,
    double& averageReturnWhenSignaled,
    double& totalReturn,
    double& averageTradeReturn,
    double& winRate,
    double& profitFactor,
    double& maxDrawdown,
    double& baseRate,
    double& naiveAccuracy,
    double& directionalAccuracy,
    double& zScore
)
{
    const std::size_t samples =
        state.validSamples;

    if (samples > 0)
    {
        mae =
            state.error /
            static_cast<double>(samples);

        baseRate =
            static_cast<double>(
                state.actualPositive
            ) /
            static_cast<double>(samples) *
            100.0;

        naiveAccuracy =
            static_cast<double>(
                state.naiveCorrect
            ) /
            static_cast<double>(samples) *
            100.0;

        directionalAccuracy =
            static_cast<double>(
                state.majorityCorrect
            ) /
            static_cast<double>(samples) *
            100.0;
    }

    if (state.signals > 0)
    {
        coverage =
            static_cast<double>(
                state.signals
            ) /
            static_cast<double>(samples) *
            100.0;

        signalAccuracy =
            static_cast<double>(
                state.correctSignals
            ) /
            static_cast<double>(state.signals) *
            100.0;

        averageReturnWhenSignaled =
            state.actualReturnSum /
            static_cast<double>(state.signals);

        averageTradeReturn =
            state.totalPnL /
            static_cast<double>(state.signals);

        winRate =
            static_cast<double>(
                state.correctSignals
            ) /
            static_cast<double>(state.signals) *
            100.0;

        profitFactor =
            state.grossLoss > 0.0
                ? state.grossProfit /
                  state.grossLoss
                : 0.0;

        zScore =
            calculateZScore(
                signalAccuracy,
                state.signals
            );
    }

    totalReturn =
        state.totalPnL;

    maxDrawdown =
        state.maxDrawdown;
}

}

BacktestMetrics runBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step
)
{
    BacktestMetrics metrics{};

    if (step == 0)
    {
        step = 1;
    }

    if (windowSize == 0 ||
        prices.size() <=
            windowSize + 30)
    {
        return metrics;
    }

    HorizonState state5{};
    HorizonState state10{};
    HorizonState state15{};
    HorizonState state30{};

    const std::size_t minimumHistory =
        windowSize + 30 + 10;

    for (std::size_t currentIndex =
             minimumHistory;
         currentIndex + 30 <
             prices.size();
         currentIndex += step)
    {
        const std::size_t currentPriceIndex =
            currentIndex +
            windowSize - 1;

        if (currentPriceIndex + 30 >=
            prices.size())
        {
            break;
        }

        std::vector<WeightedMatch> matches5;
        std::vector<WeightedMatch> matches10;
        std::vector<WeightedMatch> matches15;
        std::vector<WeightedMatch> matches30;

        const bool valid5 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                5,
                matches5
            );

        const bool valid10 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                10,
                matches10
            );

        const bool valid15 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                15,
                matches15
            );

        const bool valid30 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                30,
                matches30
            );

        if (!valid5 &&
            !valid10 &&
            !valid15 &&
            !valid30)
        {
            continue;
        }

        const double prediction5 =
            valid5
                ? calculateWeightedPrediction(
                    prices,
                    matches5,
                    windowSize,
                    5
                  )
                : 0.0;

        const double prediction10 =
            valid10
                ? calculateWeightedPrediction(
                    prices,
                    matches10,
                    windowSize,
                    10
                  )
                : 0.0;

        const double prediction15 =
            valid15
                ? calculateWeightedPrediction(
                    prices,
                    matches15,
                    windowSize,
                    15
                  )
                : 0.0;

        const double prediction30 =
            valid30
                ? calculateWeightedPrediction(
                    prices,
                    matches30,
                    windowSize,
                    30
                  )
                : 0.0;

        const double actual5 =
            calculateReturn(
                prices,
                currentPriceIndex,
                5
            );

        const double actual10 =
            calculateReturn(
                prices,
                currentPriceIndex,
                10
            );

        const double actual15 =
            calculateReturn(
                prices,
                currentPriceIndex,
                15
            );

        const double actual30 =
            calculateReturn(
                prices,
                currentPriceIndex,
                30
            );

        if (valid5)
        {
            ++state5.validSamples;
            state5.error +=
                std::abs(
                    prediction5 -
                    actual5
                );

            if (actual5 > 0.0)
            {
                ++state5.actualPositive;
            }

            if (isCorrectDirection(
                    prediction5,
                    actual5))
            {
                ++state5.majorityCorrect;
            }
        }

        if (valid10)
        {
            ++state10.validSamples;
            state10.error +=
                std::abs(
                    prediction10 -
                    actual10
                );

            if (actual10 > 0.0)
            {
                ++state10.actualPositive;
            }

            if (isCorrectDirection(
                    prediction10,
                    actual10))
            {
                ++state10.majorityCorrect;
            }
        }

        if (valid15)
        {
            ++state15.validSamples;
            state15.error +=
                std::abs(
                    prediction15 -
                    actual15
                );

            if (actual15 > 0.0)
            {
                ++state15.actualPositive;
            }

            if (isCorrectDirection(
                    prediction15,
                    actual15))
            {
                ++state15.majorityCorrect;
            }
        }

        if (valid30)
        {
            ++state30.validSamples;
            state30.error +=
                std::abs(
                    prediction30 -
                    actual30
                );

            if (actual30 > 0.0)
            {
                ++state30.actualPositive;
            }

            if (isCorrectDirection(
                    prediction30,
                    actual30))
            {
                ++state30.majorityCorrect;
            }
        }

        ++metrics.samples;
    }

    if (metrics.samples == 0)
    {
        return metrics;
    }

    metrics.mae5 =
        state5.validSamples > 0
            ? state5.error /
              state5.validSamples
            : 0.0;

    metrics.mae10 =
        state10.validSamples > 0
            ? state10.error /
              state10.validSamples
            : 0.0;

    metrics.mae15 =
        state15.validSamples > 0
            ? state15.error /
              state15.validSamples
            : 0.0;

    metrics.mae30 =
        state30.validSamples > 0
            ? state30.error /
              state30.validSamples
            : 0.0;

    metrics.directionalAccuracy5 =
        state5.validSamples > 0
            ? static_cast<double>(
                  state5.majorityCorrect
              ) /
              state5.validSamples *
              100.0
            : 0.0;

    metrics.directionalAccuracy10 =
        state10.validSamples > 0
            ? static_cast<double>(
                  state10.majorityCorrect
              ) /
              state10.validSamples *
              100.0
            : 0.0;

    metrics.directionalAccuracy15 =
        state15.validSamples > 0
            ? static_cast<double>(
                  state15.majorityCorrect
              ) /
              state15.validSamples *
              100.0
            : 0.0;

    metrics.directionalAccuracy30 =
        state30.validSamples > 0
            ? static_cast<double>(
                  state30.majorityCorrect
              ) /
              state30.validSamples *
              100.0
            : 0.0;

    metrics.baseRatePositive5 =
        state5.validSamples > 0
            ? static_cast<double>(
                  state5.actualPositive
              ) /
              state5.validSamples *
              100.0
            : 0.0;

    metrics.baseRatePositive10 =
        state10.validSamples > 0
            ? static_cast<double>(
                  state10.actualPositive
              ) /
              state10.validSamples *
              100.0
            : 0.0;

    metrics.baseRatePositive15 =
        state15.validSamples > 0
            ? static_cast<double>(
                  state15.actualPositive
              ) /
              state15.validSamples *
              100.0
            : 0.0;

    metrics.baseRatePositive30 =
        state30.validSamples > 0
            ? static_cast<double>(
                  state30.actualPositive
              ) /
              state30.validSamples *
              100.0
            : 0.0;

    metrics.zScore5 =
        calculateZScore(
            metrics.directionalAccuracy5,
            state5.validSamples
        );

    metrics.zScore10 =
        calculateZScore(
            metrics.directionalAccuracy10,
            state10.validSamples
        );

    metrics.zScore15 =
        calculateZScore(
            metrics.directionalAccuracy15,
            state15.validSamples
        );

    metrics.zScore30 =
        calculateZScore(
            metrics.directionalAccuracy30,
            state30.validSamples
        );

    return metrics;
}

BacktestMetrics runConfidenceBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step,
    double trainRatio,
    double confidenceThreshold
)
{
    BacktestMetrics metrics{};

    metrics.confidenceThreshold =
        confidenceThreshold;

    if (step == 0)
    {
        step = 1;
    }

    if (windowSize == 0 ||
        prices.size() <=
            windowSize + 30)
    {
        return metrics;
    }

    if (trainRatio <= 0.0 ||
        trainRatio >= 1.0)
    {
        return metrics;
    }

    if (confidenceThreshold < 0.5 ||
        confidenceThreshold > 1.0)
    {
        return metrics;
    }

    const std::size_t trainingEnd =
        static_cast<std::size_t>(
            static_cast<double>(prices.size()) *
            trainRatio
        );

    if (trainingEnd <= windowSize)
    {
        return metrics;
    }

    const std::size_t firstTestWindow =
        trainingEnd -
        windowSize +
        1;

    const auto calibration5 =
        buildTrainingCalibrationTable(
            prices,
            trainingEnd,
            windowSize,
            topK,
            5,
            step
        );

    const auto calibration10 =
        buildTrainingCalibrationTable(
            prices,
            trainingEnd,
            windowSize,
            topK,
            10,
            step
        );

    const auto calibration15 =
        buildTrainingCalibrationTable(
            prices,
            trainingEnd,
            windowSize,
            topK,
            15,
            step
        );

    const auto calibration30 =
        buildTrainingCalibrationTable(
            prices,
            trainingEnd,
            windowSize,
            topK,
            30,
            step
        );

    HorizonState state5{};
    HorizonState state10{};
    HorizonState state15{};
    HorizonState state30{};

    for (std::size_t currentIndex =
             firstTestWindow;
         currentIndex + 30 <
             prices.size();
         currentIndex += step)
    {
        const std::size_t currentPriceIndex =
            currentIndex +
            windowSize - 1;

        if (currentPriceIndex + 30 >=
            prices.size())
        {
            break;
        }

        std::vector<WeightedMatch> matches5;
        std::vector<WeightedMatch> matches10;
        std::vector<WeightedMatch> matches15;
        std::vector<WeightedMatch> matches30;

        const bool valid5 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                5,
                matches5
            );

        const bool valid10 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                10,
                matches10
            );

        const bool valid15 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                15,
                matches15
            );

        const bool valid30 =
            buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                30,
                matches30
            );

        if (!valid5 &&
            !valid10 &&
            !valid15 &&
            !valid30)
        {
            continue;
        }

        const double actual5 =
            calculateReturn(
                prices,
                currentPriceIndex,
                5
            );

        const double actual10 =
            calculateReturn(
                prices,
                currentPriceIndex,
                10
            );

        const double actual15 =
            calculateReturn(
                prices,
                currentPriceIndex,
                15
            );

        const double actual30 =
            calculateReturn(
                prices,
                currentPriceIndex,
                30
            );

        const double prediction5 =
            valid5
                ? calculateWeightedPrediction(
                    prices,
                    matches5,
                    windowSize,
                    5
                  )
                : 0.0;

        const double prediction10 =
            valid10
                ? calculateWeightedPrediction(
                    prices,
                    matches10,
                    windowSize,
                    10
                  )
                : 0.0;

        const double prediction15 =
            valid15
                ? calculateWeightedPrediction(
                    prices,
                    matches15,
                    windowSize,
                    15
                  )
                : 0.0;

        const double prediction30 =
            valid30
                ? calculateWeightedPrediction(
                    prices,
                    matches30,
                    windowSize,
                    30
                  )
                : 0.0;

        ConfidenceResult confidence5{};
        ConfidenceResult confidence10{};
        ConfidenceResult confidence15{};
        ConfidenceResult confidence30{};

        double calibrated5 = 0.0;
        double calibrated10 = 0.0;
        double calibrated15 = 0.0;
        double calibrated30 = 0.0;

        if (valid5)
        {
            confidence5 =
                calculateConfidence(
                    prices,
                    matches5,
                    windowSize,
                    confidenceThreshold
                );

            calibrated5 =
                lookupCalibratedConfidence(
                    calibration5,
                    confidence5.confidence5.confidence
                );
        }

        if (valid10)
        {
            confidence10 =
                calculateConfidence(
                    prices,
                    matches10,
                    windowSize,
                    confidenceThreshold
                );

            calibrated10 =
                lookupCalibratedConfidence(
                    calibration10,
                    confidence10.confidence10.confidence
                );
        }

        if (valid15)
        {
            confidence15 =
                calculateConfidence(
                    prices,
                    matches15,
                    windowSize,
                    confidenceThreshold
                );

            calibrated15 =
                lookupCalibratedConfidence(
                    calibration15,
                    confidence15.confidence15.confidence
                );
        }

        if (valid30)
        {
            confidence30 =
                calculateConfidence(
                    prices,
                    matches30,
                    windowSize,
                    confidenceThreshold
                );

            calibrated30 =
                lookupCalibratedConfidence(
                    calibration30,
                    confidence30.confidence30.confidence
                );
        }

        const double agreement =
            calculatePredictionAgreement(
                valid5,
                valid5 &&
                    confidence5.confidence5.predictedPositive,

                valid10,
                valid10 &&
                    confidence10.confidence10.predictedPositive,

                valid15,
                valid15 &&
                    confidence15.confidence15.predictedPositive,

                valid30,
                valid30 &&
                    confidence30.confidence30.predictedPositive
            );

        constexpr double MIN_AGREEMENT = 0.75;

        const bool signal5 =
            valid5 &&
            calibrated5 >= confidenceThreshold &&
            agreement >= MIN_AGREEMENT;

        const bool signal10 =
            valid10 &&
            calibrated10 >= confidenceThreshold &&
            agreement >= MIN_AGREEMENT;

        const bool signal15 =
            valid15 &&
            calibrated15 >= confidenceThreshold &&
            agreement >= MIN_AGREEMENT;

        const bool signal30 =
            valid30 &&
            calibrated30 >= confidenceThreshold &&
            agreement >= MIN_AGREEMENT;

        if (valid5)
        {
            ++state5.validSamples;
            state5.error +=
                std::abs(
                    prediction5 -
                    actual5
                );

            if (actual5 > 0.0)
            {
                ++state5.actualPositive;
            }

            if (signal5)
            {
                const bool positive =
                    confidence5
                        .confidence5
                        .predictedPositive;

                const double tradeReturn =
                    calculateDirectionalTradeReturn(
                        prices,
                        currentPriceIndex,
                        5,
                        positive
                    );

                updateSignalStatistics(
                    state5,
                    actual5,
                    tradeReturn
                );
            }
        }

        if (valid10)
        {
            ++state10.validSamples;
            state10.error +=
                std::abs(
                    prediction10 -
                    actual10
                );

            if (actual10 > 0.0)
            {
                ++state10.actualPositive;
            }

            if (signal10)
            {
                const bool positive =
                    confidence10
                        .confidence10
                        .predictedPositive;

                const double tradeReturn =
                    calculateDirectionalTradeReturn(
                        prices,
                        currentPriceIndex,
                        10,
                        positive
                    );

                updateSignalStatistics(
                    state10,
                    actual10,
                    tradeReturn
                );
            }
        }

        if (valid15)
        {
            ++state15.validSamples;
            state15.error +=
                std::abs(
                    prediction15 -
                    actual15
                );

            if (actual15 > 0.0)
            {
                ++state15.actualPositive;
            }

            if (signal15)
            {
                const bool positive =
                    confidence15
                        .confidence15
                        .predictedPositive;

                const double tradeReturn =
                    calculateDirectionalTradeReturn(
                        prices,
                        currentPriceIndex,
                        15,
                        positive
                    );

                updateSignalStatistics(
                    state15,
                    actual15,
                    tradeReturn
                );
            }
        }

        if (valid30)
        {
            ++state30.validSamples;
            state30.error +=
                std::abs(
                    prediction30 -
                    actual30
                );

            if (actual30 > 0.0)
            {
                ++state30.actualPositive;
            }

            if (signal30)
            {
                const bool positive =
                    confidence30
                        .confidence30
                        .predictedPositive;

                const double tradeReturn =
                    calculateDirectionalTradeReturn(
                        prices,
                        currentPriceIndex,
                        30,
                        positive
                    );

                updateSignalStatistics(
                    state30,
                    actual30,
                    tradeReturn
                );
            }
        }

        const double trailingReturn =
            calculateTrailingReturn(
                prices,
                currentPriceIndex,
                5
            );

        const bool momentumPositive =
            trailingReturn >= 0.0;

        const MajorityCounts majority5 =
            calculateMajorityCounts(
                prices,
                currentIndex,
                windowSize,
                5
            );

        const MajorityCounts majority10 =
            calculateMajorityCounts(
                prices,
                currentIndex,
                windowSize,
                10
            );

        const MajorityCounts majority15 =
            calculateMajorityCounts(
                prices,
                currentIndex,
                windowSize,
                15
            );

        const MajorityCounts majority30 =
            calculateMajorityCounts(
                prices,
                currentIndex,
                windowSize,
                30
            );

        if (valid5)
        {
            if (
                (momentumPositive &&
                 actual5 > 0.0) ||
                (!momentumPositive &&
                 actual5 < 0.0)
            )
            {
                ++state5.naiveCorrect;
            }

            if (
                (majorityPrediction(majority5) &&
                 actual5 > 0.0) ||
                (!majorityPrediction(majority5) &&
                 actual5 < 0.0)
            )
            {
                ++state5.majorityCorrect;
            }
        }

        if (valid10)
        {
            if (
                (momentumPositive &&
                 actual10 > 0.0) ||
                (!momentumPositive &&
                 actual10 < 0.0)
            )
            {
                ++state10.naiveCorrect;
            }

            if (
                (majorityPrediction(majority10) &&
                 actual10 > 0.0) ||
                (!majorityPrediction(majority10) &&
                 actual10 < 0.0)
            )
            {
                ++state10.majorityCorrect;
            }
        }

        if (valid15)
        {
            if (
                (momentumPositive &&
                 actual15 > 0.0) ||
                (!momentumPositive &&
                 actual15 < 0.0)
            )
            {
                ++state15.naiveCorrect;
            }

            if (
                (majorityPrediction(majority15) &&
                 actual15 > 0.0) ||
                (!majorityPrediction(majority15) &&
                 actual15 < 0.0)
            )
            {
                ++state15.majorityCorrect;
            }
        }

        if (valid30)
        {
            if (
                (momentumPositive &&
                 actual30 > 0.0) ||
                (!momentumPositive &&
                 actual30 < 0.0)
            )
            {
                ++state30.naiveCorrect;
            }

            if (
                (majorityPrediction(majority30) &&
                 actual30 > 0.0) ||
                (!majorityPrediction(majority30) &&
                 actual30 < 0.0)
            )
            {
                ++state30.majorityCorrect;
            }
        }

        ++metrics.samples;

        if (metrics.samples <= 5)
        {
            std::cout
                << "\nWalk-forward test sample "
                << metrics.samples
                << "\n";

            std::cout
                << "Query index: "
                << currentIndex
                << "\n";

            std::cout
                << "Prediction price index: "
                << currentPriceIndex
                << "\n";

            if (valid5)
            {
                std::cout
                    << "+5  Predicted: "
                    << prediction5
                    << "% | Actual: "
                    << actual5
                    << "% | Raw confidence: "
                    << confidence5.confidence5.confidence * 100.0
                    << "% | Calibrated: "
                    << calibrated5 * 100.0
                    << "% | "
                    << (
                        signal5
                            ? "SIGNAL"
                            : "NO SIGNAL"
                    )
                    << "\n";
            }

            if (valid10)
            {
                std::cout
                    << "+10 Predicted: "
                    << prediction10
                    << "% | Actual: "
                    << actual10
                    << "% | Raw confidence: "
                    << confidence10.confidence10.confidence * 100.0
                    << "% | Calibrated: "
                    << calibrated10 * 100.0
                    << "% | "
                    << (
                        signal10
                            ? "SIGNAL"
                            : "NO SIGNAL"
                    )
                    << "\n";
            }

            if (valid15)
            {
                std::cout
                    << "+15 Predicted: "
                    << prediction15
                    << "% | Actual: "
                    << actual15
                    << "% | Raw confidence: "
                    << confidence15.confidence15.confidence * 100.0
                    << "% | Calibrated: "
                    << calibrated15 * 100.0
                    << "% | "
                    << (
                        signal15
                            ? "SIGNAL"
                            : "NO SIGNAL"
                    )
                    << "\n";
            }

            if (valid30)
            {
                std::cout
                    << "+30 Predicted: "
                    << prediction30
                    << "% | Actual: "
                    << actual30
                    << "% | Raw confidence: "
                    << confidence30.confidence30.confidence * 100.0
                    << "% | Calibrated: "
                    << calibrated30 * 100.0
                    << "% | "
                    << (
                        signal30
                            ? "SIGNAL"
                            : "NO SIGNAL"
                    )
                    << "\n";
            }

            std::cout
                << "Prediction agreement: "
                << agreement * 100.0
                << "%\n";
        }
    }

    if (metrics.samples == 0)
    {
        return metrics;
    }

    metrics.signals5 =
        state5.signals;

    metrics.signals10 =
        state10.signals;

    metrics.signals15 =
        state15.signals;

    metrics.signals30 =
        state30.signals;

    metrics.mae5 =
        state5.validSamples > 0
            ? state5.error /
              state5.validSamples
            : 0.0;

    metrics.mae10 =
        state10.validSamples > 0
            ? state10.error /
              state10.validSamples
            : 0.0;

    metrics.mae15 =
        state15.validSamples > 0
            ? state15.error /
              state15.validSamples
            : 0.0;

    metrics.mae30 =
        state30.validSamples > 0
            ? state30.error /
              state30.validSamples
            : 0.0;

    metrics.coverage5 =
        static_cast<double>(
            state5.signals
        ) /
        static_cast<double>(
            state5.validSamples
        ) * 100.0;

    metrics.coverage10 =
        static_cast<double>(
            state10.signals
        ) /
        static_cast<double>(
            state10.validSamples
        ) * 100.0;

    metrics.coverage15 =
        static_cast<double>(
            state15.signals
        ) /
        static_cast<double>(
            state15.validSamples
        ) * 100.0;

    metrics.coverage30 =
        static_cast<double>(
            state30.signals
        ) /
        static_cast<double>(
            state30.validSamples
        ) * 100.0;

    if (state5.signals > 0)
    {
        metrics.signalAccuracy5 =
            static_cast<double>(
                state5.correctSignals
            ) /
            state5.signals *
            100.0;

        metrics.averageReturnWhenSignaled5 =
            state5.actualReturnSum /
            state5.signals;

        metrics.averageTradeReturn5 =
            state5.totalPnL /
            state5.signals;

        metrics.winRate5 =
            metrics.signalAccuracy5;

        metrics.profitFactor5 =
            state5.grossLoss > 0.0
                ? state5.grossProfit /
                  state5.grossLoss
                : 0.0;
    }

    if (state10.signals > 0)
    {
        metrics.signalAccuracy10 =
            static_cast<double>(
                state10.correctSignals
            ) /
            state10.signals *
            100.0;

        metrics.averageReturnWhenSignaled10 =
            state10.actualReturnSum /
            state10.signals;

        metrics.averageTradeReturn10 =
            state10.totalPnL /
            state10.signals;

        metrics.winRate10 =
            metrics.signalAccuracy10;

        metrics.profitFactor10 =
            state10.grossLoss > 0.0
                ? state10.grossProfit /
                  state10.grossLoss
                : 0.0;
    }

    if (state15.signals > 0)
    {
        metrics.signalAccuracy15 =
            static_cast<double>(
                state15.correctSignals
            ) /
            state15.signals *
            100.0;

        metrics.averageReturnWhenSignaled15 =
            state15.actualReturnSum /
            state15.signals;

        metrics.averageTradeReturn15 =
            state15.totalPnL /
            state15.signals;

        metrics.winRate15 =
            metrics.signalAccuracy15;

        metrics.profitFactor15 =
            state15.grossLoss > 0.0
                ? state15.grossProfit /
                  state15.grossLoss
                : 0.0;
    }

    if (state30.signals > 0)
    {
        metrics.signalAccuracy30 =
            static_cast<double>(
                state30.correctSignals
            ) /
            state30.signals *
            100.0;

        metrics.averageReturnWhenSignaled30 =
            state30.actualReturnSum /
            state30.signals;

        metrics.averageTradeReturn30 =
            state30.totalPnL /
            state30.signals;

        metrics.winRate30 =
            metrics.signalAccuracy30;

        metrics.profitFactor30 =
            state30.grossLoss > 0.0
                ? state30.grossProfit /
                  state30.grossLoss
                : 0.0;
    }

    metrics.totalReturn5 =
        state5.totalPnL;

    metrics.totalReturn10 =
        state10.totalPnL;

    metrics.totalReturn15 =
        state15.totalPnL;

    metrics.totalReturn30 =
        state30.totalPnL;

    metrics.maxDrawdown5 =
        state5.maxDrawdown;

    metrics.maxDrawdown10 =
        state10.maxDrawdown;

    metrics.maxDrawdown15 =
        state15.maxDrawdown;

    metrics.maxDrawdown30 =
        state30.maxDrawdown;

    metrics.baseRatePositive5 =
        static_cast<double>(
            state5.actualPositive
        ) /
        state5.validSamples *
        100.0;

    metrics.baseRatePositive10 =
        static_cast<double>(
            state10.actualPositive
        ) /
        state10.validSamples *
        100.0;

    metrics.baseRatePositive15 =
        static_cast<double>(
            state15.actualPositive
        ) /
        state15.validSamples *
        100.0;

    metrics.baseRatePositive30 =
        static_cast<double>(
            state30.actualPositive
        ) /
        state30.validSamples *
        100.0;

    metrics.naiveAccuracy5 =
        static_cast<double>(
            state5.naiveCorrect
        ) /
        state5.validSamples *
        100.0;

    metrics.naiveAccuracy10 =
        static_cast<double>(
            state10.naiveCorrect
        ) /
        state10.validSamples *
        100.0;

    metrics.naiveAccuracy15 =
        static_cast<double>(
            state15.naiveCorrect
        ) /
        state15.validSamples *
        100.0;

    metrics.naiveAccuracy30 =
        static_cast<double>(
            state30.naiveCorrect
        ) /
        state30.validSamples *
        100.0;

    metrics.directionalAccuracy5 =
        static_cast<double>(
            state5.majorityCorrect
        ) /
        state5.validSamples *
        100.0;

    metrics.directionalAccuracy10 =
        static_cast<double>(
            state10.majorityCorrect
        ) /
        state10.validSamples *
        100.0;

    metrics.directionalAccuracy15 =
        static_cast<double>(
            state15.majorityCorrect
        ) /
        state15.validSamples *
        100.0;

    metrics.directionalAccuracy30 =
        static_cast<double>(
            state30.majorityCorrect
        ) /
        state30.validSamples *
        100.0;

    metrics.zScore5 =
        calculateZScore(
            metrics.signalAccuracy5,
            state5.signals
        );

    metrics.zScore10 =
        calculateZScore(
            metrics.signalAccuracy10,
            state10.signals
        );

    metrics.zScore15 =
        calculateZScore(
            metrics.signalAccuracy15,
            state15.signals
        );

    metrics.zScore30 =
        calculateZScore(
            metrics.signalAccuracy30,
            state30.signals
        );

    return metrics;
}