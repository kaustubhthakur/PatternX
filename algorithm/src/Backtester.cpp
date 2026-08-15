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

double calculateReturn(
    const std::vector<double>& prices,
    std::size_t startPrice,
    std::size_t days
)
{
    const std::size_t futureIndex =
        startPrice + days;

    if (futureIndex >= prices.size())
    {
        return 0.0;
    }

    const double currentPrice =
        prices[startPrice];

    const double futurePrice =
        prices[futureIndex];

    if (currentPrice == 0.0)
    {
        return 0.0;
    }

    return ((futurePrice - currentPrice)
            / currentPrice) * 100.0;
}


double calculateTrailingReturn(
    const std::vector<double>& prices,
    std::size_t priceIndex,
    std::size_t days
)
{
    if (priceIndex < days)
    {
        return 0.0;
    }

    const std::size_t pastIndex =
        priceIndex - days;

    const double pastPrice =
        prices[pastIndex];

    const double currentPrice =
        prices[priceIndex];

    if (pastPrice == 0.0)
    {
        return 0.0;
    }

    return ((currentPrice - pastPrice)
            / pastPrice) * 100.0;
}


double calculateZScore(
    double accuracyPercent,
    std::size_t n
)
{
    if (n == 0)
    {
        return 0.0;
    }

    const double observed =
        accuracyPercent / 100.0;

    const double standardError =
        std::sqrt(
            0.25 /
            static_cast<double>(n)
        );

    if (standardError == 0.0)
    {
        return 0.0;
    }

    return (observed - 0.5)
           / standardError;
}


/*
    Calculate the actual trading return.

    If predictedPositive == true:
        Long position.

    If predictedPositive == false:
        Short position.

    The returned value represents the
    directional strategy return.
*/
double calculateDirectionalTradeReturn(
    const std::vector<double>& prices,
    std::size_t currentPriceIndex,
    std::size_t horizon,
    bool predictedPositive
)
{
    const std::size_t futureIndex =
        currentPriceIndex + horizon;

    if (futureIndex >= prices.size())
    {
        return 0.0;
    }

    const double currentPrice =
        prices[currentPriceIndex];

    const double futurePrice =
        prices[futureIndex];

    if (currentPrice == 0.0)
    {
        return 0.0;
    }

    const double rawReturn =
        ((futurePrice - currentPrice)
         / currentPrice) * 100.0;

    if (predictedPositive)
    {
        return rawReturn;
    }

    return -rawReturn;
}


/*
    Build the pattern universe available at a
    particular query index.

    A historical window is allowed only if its
    complete future outcome is already known
    before the query begins.
*/
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

    if (currentIndex + windowSize >
        prices.size())
    {
        return false;
    }

    std::vector<std::vector<double>>
        signatures;

    std::vector<std::vector<double>>
        windows;

    std::vector<std::size_t>
        historicalIndices;

    const std::size_t totalWindows =
        currentIndex + 1;

    signatures.reserve(totalWindows);
    windows.reserve(totalWindows);
    historicalIndices.reserve(totalWindows);

    for (std::size_t start = 0;
         start <= currentIndex;
         ++start)
    {
        if (start + windowSize >
            prices.size())
        {
            break;
        }

        std::vector<double> window(
            prices.begin() + start,
            prices.begin() + start + windowSize
        );

        std::vector<double> normalized =
            normalizeWindow(window);

        std::vector<std::complex<double>>
            fftResult =
                computeFFT(normalized);

        std::vector<double> magnitude =
            computeMagnitude(fftResult);

        signatures.push_back(magnitude);
        windows.push_back(window);
        historicalIndices.push_back(start);
    }

    if (signatures.empty())
    {
        return false;
    }

    const std::vector<double>&
        currentSignature =
            signatures[currentIndex];

    const std::vector<double>&
        currentWindow =
            windows[currentIndex];

    std::vector<std::vector<double>>
        historicalSignatures;

    std::vector<std::vector<double>>
        historicalWindows;

    std::vector<std::size_t>
        candidateIndices;

    /*
        STRICT walk-forward restriction.

        Historical pattern's complete future
        outcome must exist before currentIndex.
    */
    for (std::size_t j = 0;
         j < currentIndex;
         ++j)
    {
        const std::size_t futureIndex =
            j + windowSize - 1 + horizon;

        if (futureIndex >= currentIndex)
        {
            continue;
        }

        historicalSignatures.push_back(
            signatures[j]
        );

        historicalWindows.push_back(
            windows[j]
        );

        candidateIndices.push_back(j);
    }

    if (historicalSignatures.empty())
    {
        return false;
    }

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

    std::vector<std::size_t>
        windowIndices;

    std::vector<double>
        distances;

    windowIndices.reserve(matches.size());
    distances.reserve(matches.size());

    for (const auto& match : matches)
    {
        /*
            match.windowIndex is the index inside
            historicalSignatures.

            Convert it back to the original
            price-window index.
        */
        if (match.windowIndex >=
            candidateIndices.size())
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



/*
    Build a calibration table from observations that are fully
    contained inside the training period.

    IMPORTANT:
    A calibration point at queryIndex is only created when the
    complete future outcome for the selected horizon is already
    known before trainingEnd. This prevents future/test leakage.

    Calibration is built separately for each horizon because the
    confidence score has a different meaning at +5/+10/+15/+30.
*/
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

    if (trainingEnd <= windowSize)
    {
        return points;
    }

    if (step == 0)
    {
        step = 1;
    }

    /*
        Leave enough history for matching and enough room for the
        complete horizon outcome to be known inside the training set.
    */
    const std::size_t minimumQueryIndex =
        windowSize + horizon + 1;

    if (trainingEnd <= minimumQueryIndex)
    {
        return points;
    }

    /*
        Use only query points whose complete outcome lies strictly
        before the training boundary.

        We deliberately use the same step as the backtest so the
        calibration sample is computationally manageable and has
        the same temporal sampling characteristics.
    */
    for (std::size_t queryIndex = minimumQueryIndex;
         queryIndex < trainingEnd;
         queryIndex += step)
    {
        const std::size_t predictionIndex =
            queryIndex + windowSize - 1;

        if (predictionIndex >= trainingEnd)
        {
            break;
        }

        /*
            The historical outcome must be completely known inside
            the training period.
        */
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

        /*
            Use the existing PatternX confidence calculation.

            The threshold is irrelevant here because we only need
            the raw confidence value. Signal generation is performed
            later using the calibrated confidence.
        */
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

        /*
            A calibration point records whether the directional
            prediction was correct.
        */
        const bool wasCorrect =
            (predictedPositive && actualReturn > 0.0) ||
            (!predictedPositive && actualReturn < 0.0);

        points.push_back(
            CalibrationPoint{
                rawConfidence,
                wasCorrect
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
    const std::vector<CalibrationPoint> points =
        collectCalibrationPoints(
            prices,
            trainingEnd,
            windowSize,
            topK,
            horizon,
            step
        );

    /*
        Ten buckets are used by default, but the Calibration module
        itself will safely return an empty table when insufficient
        training observations are available.
    */
    return buildCalibrationTable(
        points,
        10
    );
}



/*
    ============================================================
    PREDICTION AGREEMENT
    ============================================================

    Measures how strongly the four horizons agree on direction.

    Example:
        +5  UP
        +10 UP
        +15 UP
        +30 DOWN

    Agreement = 3 / 4 = 75%.

    A horizon receives the agreement of the group that shares
    its direction. This is intentionally direction-only; the
    confidence and expected-return magnitude remain separate
    signals.

    With four horizons:
        100% = 4/4 agree
         75% = 3/4 agree
         50% = 2/4 split
         25% = 1/4
*/
double calculatePredictionAgreement(
    bool valid5,
    bool predictedPositive5,
    bool valid10,
    bool predictedPositive10,
    bool valid15,
    bool predictedPositive15,
    bool valid30,
    bool predictedPositive30
)
{
    std::size_t validCount = 0;
    std::size_t positiveCount = 0;

    if (valid5)
    {
        ++validCount;
        if (predictedPositive5)
        {
            ++positiveCount;
        }
    }

    if (valid10)
    {
        ++validCount;
        if (predictedPositive10)
        {
            ++positiveCount;
        }
    }

    if (valid15)
    {
        ++validCount;
        if (predictedPositive15)
        {
            ++positiveCount;
        }
    }

    if (valid30)
    {
        ++validCount;
        if (predictedPositive30)
        {
            ++positiveCount;
        }
    }

    if (validCount == 0)
    {
        return 0.0;
    }

    const std::size_t negativeCount =
        validCount - positiveCount;

    const std::size_t dominantCount =
        std::max(
            positiveCount,
            negativeCount
        );

    return static_cast<double>(dominantCount) /
           static_cast<double>(validCount);
}


double calculateWeightedPredictionFromMatches(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& matches,
    std::size_t windowSize,
    std::size_t horizon
)
{
    double prediction = 0.0;

    for (const auto& match : matches)
    {
        const std::size_t historicalWindow =
            match.windowIndex;

        const std::size_t endPriceIndex =
            historicalWindow +
            windowSize - 1;

        const double historicalReturn =
            calculateReturn(
                prices,
                endPriceIndex,
                horizon
            );

        prediction +=
            match.normalizedWeight *
            historicalReturn;
    }

    return prediction;
}

}


/*
============================================================
EXISTING BACKTEST
============================================================
*/
BacktestMetrics runBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step
)
{
    BacktestMetrics metrics{};

    if (prices.size() <
        windowSize + 30)
    {
        return metrics;
    }

    if (step == 0)
    {
        step = 1;
    }

    double error5 = 0.0;
    double error10 = 0.0;
    double error15 = 0.0;
    double error30 = 0.0;

    std::size_t correct5 = 0;
    std::size_t correct10 = 0;
    std::size_t correct15 = 0;
    std::size_t correct30 = 0;

    std::size_t actualPositive5 = 0;
    std::size_t actualPositive10 = 0;
    std::size_t actualPositive15 = 0;
    std::size_t actualPositive30 = 0;

    std::size_t naiveCorrect5 = 0;
    std::size_t naiveCorrect10 = 0;
    std::size_t naiveCorrect15 = 0;
    std::size_t naiveCorrect30 = 0;

    const std::size_t MIN_HISTORY =
        windowSize + 30 + 10;

    for (std::size_t currentIndex = MIN_HISTORY;
         currentIndex + 30 < prices.size();
         currentIndex += step)
    {
        std::vector<WeightedMatch>
            matches5;

        std::vector<WeightedMatch>
            matches10;

        std::vector<WeightedMatch>
            matches15;

        std::vector<WeightedMatch>
            matches30;

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
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches5,
                    windowSize,
                    5
                  )
                : 0.0;

        const double prediction10 =
            valid10
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches10,
                    windowSize,
                    10
                  )
                : 0.0;

        const double prediction15 =
            valid15
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches15,
                    windowSize,
                    15
                  )
                : 0.0;

        const double prediction30 =
            valid30
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches30,
                    windowSize,
                    30
                  )
                : 0.0;

        const std::size_t currentPriceIndex =
            currentIndex +
            windowSize - 1;

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
            error5 +=
                std::abs(
                    prediction5 - actual5
                );

            if (
                (prediction5 >= 0.0 &&
                 actual5 >= 0.0) ||
                (prediction5 < 0.0 &&
                 actual5 < 0.0)
            )
            {
                ++correct5;
            }

            if (actual5 >= 0.0)
                ++actualPositive5;
        }

        if (valid10)
        {
            error10 +=
                std::abs(
                    prediction10 - actual10
                );

            if (
                (prediction10 >= 0.0 &&
                 actual10 >= 0.0) ||
                (prediction10 < 0.0 &&
                 actual10 < 0.0)
            )
            {
                ++correct10;
            }

            if (actual10 >= 0.0)
                ++actualPositive10;
        }

        if (valid15)
        {
            error15 +=
                std::abs(
                    prediction15 - actual15
                );

            if (
                (prediction15 >= 0.0 &&
                 actual15 >= 0.0) ||
                (prediction15 < 0.0 &&
                 actual15 < 0.0)
            )
            {
                ++correct15;
            }

            if (actual15 >= 0.0)
                ++actualPositive15;
        }

        if (valid30)
        {
            error30 +=
                std::abs(
                    prediction30 - actual30
                );

            if (
                (prediction30 >= 0.0 &&
                 actual30 >= 0.0) ||
                (prediction30 < 0.0 &&
                 actual30 < 0.0)
            )
            {
                ++correct30;
            }

            if (actual30 >= 0.0)
                ++actualPositive30;
        }

        const double trailingReturn =
            calculateTrailingReturn(
                prices,
                currentPriceIndex,
                5
            );

        if (
            valid5 &&
            (
                (trailingReturn >= 0.0 &&
                 actual5 >= 0.0) ||
                (trailingReturn < 0.0 &&
                 actual5 < 0.0)
            )
        )
        {
            ++naiveCorrect5;
        }

        if (
            valid10 &&
            (
                (trailingReturn >= 0.0 &&
                 actual10 >= 0.0) ||
                (trailingReturn < 0.0 &&
                 actual10 < 0.0)
            )
        )
        {
            ++naiveCorrect10;
        }

        if (
            valid15 &&
            (
                (trailingReturn >= 0.0 &&
                 actual15 >= 0.0) ||
                (trailingReturn < 0.0 &&
                 actual15 < 0.0)
            )
        )
        {
            ++naiveCorrect15;
        }

        if (
            valid30 &&
            (
                (trailingReturn >= 0.0 &&
                 actual30 >= 0.0) ||
                (trailingReturn < 0.0 &&
                 actual30 < 0.0)
            )
        )
        {
            ++naiveCorrect30;
        }

        ++metrics.samples;

        if (metrics.samples <= 5)
        {
            std::cout
                << "\nBacktest sample "
                << metrics.samples
                << "\n";

            std::cout
                << "+5  Predicted: "
                << prediction5
                << "% | Actual: "
                << actual5
                << "%\n";

            std::cout
                << "+10 Predicted: "
                << prediction10
                << "% | Actual: "
                << actual10
                << "%\n";

            std::cout
                << "+15 Predicted: "
                << prediction15
                << "% | Actual: "
                << actual15
                << "%\n";

            std::cout
                << "+30 Predicted: "
                << prediction30
                << "% | Actual: "
                << actual30
                << "%\n";
        }
    }

    if (metrics.samples == 0)
    {
        return metrics;
    }

    metrics.mae5 =
        error5 /
        metrics.samples;

    metrics.mae10 =
        error10 /
        metrics.samples;

    metrics.mae15 =
        error15 /
        metrics.samples;

    metrics.mae30 =
        error30 /
        metrics.samples;

    metrics.directionalAccuracy5 =
        (static_cast<double>(correct5) /
         metrics.samples) * 100.0;

    metrics.directionalAccuracy10 =
        (static_cast<double>(correct10) /
         metrics.samples) * 100.0;

    metrics.directionalAccuracy15 =
        (static_cast<double>(correct15) /
         metrics.samples) * 100.0;

    metrics.directionalAccuracy30 =
        (static_cast<double>(correct30) /
         metrics.samples) * 100.0;

    metrics.baseRatePositive5 =
        (static_cast<double>(actualPositive5) /
         metrics.samples) * 100.0;

    metrics.baseRatePositive10 =
        (static_cast<double>(actualPositive10) /
         metrics.samples) * 100.0;

    metrics.baseRatePositive15 =
        (static_cast<double>(actualPositive15) /
         metrics.samples) * 100.0;

    metrics.baseRatePositive30 =
        (static_cast<double>(actualPositive30) /
         metrics.samples) * 100.0;

    metrics.naiveAccuracy5 =
        (static_cast<double>(naiveCorrect5) /
         metrics.samples) * 100.0;

    metrics.naiveAccuracy10 =
        (static_cast<double>(naiveCorrect10) /
         metrics.samples) * 100.0;

    metrics.naiveAccuracy15 =
        (static_cast<double>(naiveCorrect15) /
         metrics.samples) * 100.0;

    metrics.naiveAccuracy30 =
        (static_cast<double>(naiveCorrect30) /
         metrics.samples) * 100.0;

    metrics.zScore5 =
        calculateZScore(
            metrics.directionalAccuracy5,
            metrics.samples
        );

    metrics.zScore10 =
        calculateZScore(
            metrics.directionalAccuracy10,
            metrics.samples
        );

    metrics.zScore15 =
        calculateZScore(
            metrics.directionalAccuracy15,
            metrics.samples
        );

    metrics.zScore30 =
        calculateZScore(
            metrics.directionalAccuracy30,
            metrics.samples
        );

    return metrics;
}


/*
============================================================
CONFIDENCE WALK-FORWARD BACKTEST
============================================================
*/
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

    if (prices.size() <
        windowSize + 30)
    {
        return metrics;
    }

    if (step == 0)
    {
        step = 1;
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

    /*
        Initial training boundary.

        This is a PRICE INDEX boundary.
    */
    const std::size_t initialTrainEnd =
        static_cast<std::size_t>(
            static_cast<double>(prices.size())
            * trainRatio
        );

    if (initialTrainEnd < windowSize)
    {
        return metrics;
    }

    /*
        The query window ends exactly at the
        initial training boundary.

        Example:

            window = 30
            trainEnd = 1731

            query = 1702 -> 1731
            prediction point = 1731
    */
    const std::size_t firstTestWindow =
        initialTrainEnd -
        windowSize +
        1;

    std::cout
        << "\nTrain ratio : "
        << trainRatio * 100.0
        << "%\n";

    std::cout
        << "Initial train end index : "
        << initialTrainEnd
        << "\n";

    std::cout
        << "First test window index : "
        << firstTestWindow
        << "\n";

    /*
        ========================================================
        TRAINING-ONLY CONFIDENCE CALIBRATION
        ========================================================

        Calibration is built once from the initial training period.
        No test-period observations are used.

        Separate calibration tables are maintained for each horizon
        because confidence at +5 days does not necessarily have the
        same empirical meaning as confidence at +30 days.
    */
    const std::vector<CalibrationBucket> calibration5 =
        buildTrainingCalibrationTable(
            prices,
            initialTrainEnd,
            windowSize,
            topK,
            5,
            step
        );

    const std::vector<CalibrationBucket> calibration10 =
        buildTrainingCalibrationTable(
            prices,
            initialTrainEnd,
            windowSize,
            topK,
            10,
            step
        );

    const std::vector<CalibrationBucket> calibration15 =
        buildTrainingCalibrationTable(
            prices,
            initialTrainEnd,
            windowSize,
            topK,
            15,
            step
        );

    const std::vector<CalibrationBucket> calibration30 =
        buildTrainingCalibrationTable(
            prices,
            initialTrainEnd,
            windowSize,
            topK,
            30,
            step
        );

    std::cout
        << "Calibration buckets (+5/+10/+15/+30): "
        << calibration5.size()
        << "/"
        << calibration10.size()
        << "/"
        << calibration15.size()
        << "/"
        << calibration30.size()
        << "\n";

    /*
        Signal statistics.
    */
    std::size_t signals5 = 0;
    std::size_t signals10 = 0;
    std::size_t signals15 = 0;
    std::size_t signals30 = 0;

    std::size_t correctSignals5 = 0;
    std::size_t correctSignals10 = 0;
    std::size_t correctSignals15 = 0;
    std::size_t correctSignals30 = 0;

    double returnWhenSignaled5 = 0.0;
    double returnWhenSignaled10 = 0.0;
    double returnWhenSignaled15 = 0.0;
    double returnWhenSignaled30 = 0.0;

    /*
        P&L statistics.
    */
    double totalPnL5 = 0.0;
    double totalPnL10 = 0.0;
    double totalPnL15 = 0.0;
    double totalPnL30 = 0.0;

    double grossProfit5 = 0.0;
    double grossProfit10 = 0.0;
    double grossProfit15 = 0.0;
    double grossProfit30 = 0.0;

    double grossLoss5 = 0.0;
    double grossLoss10 = 0.0;
    double grossLoss15 = 0.0;
    double grossLoss30 = 0.0;

    /*
        Equity curves for drawdown.
    */
    double equity5 = 0.0;
    double equity10 = 0.0;
    double equity15 = 0.0;
    double equity30 = 0.0;

    double peakEquity5 = 0.0;
    double peakEquity10 = 0.0;
    double peakEquity15 = 0.0;
    double peakEquity30 = 0.0;

    double maxDrawdown5 = 0.0;
    double maxDrawdown10 = 0.0;
    double maxDrawdown15 = 0.0;
    double maxDrawdown30 = 0.0;

    /*
        Training-only majority statistics.
    */
    std::size_t majorityPositive5 = 0;
    std::size_t majorityNegative5 = 0;

    std::size_t majorityPositive10 = 0;
    std::size_t majorityNegative10 = 0;

    std::size_t majorityPositive15 = 0;
    std::size_t majorityNegative15 = 0;

    std::size_t majorityPositive30 = 0;
    std::size_t majorityNegative30 = 0;

    std::size_t majorityCorrect5 = 0;
    std::size_t majorityCorrect10 = 0;
    std::size_t majorityCorrect15 = 0;
    std::size_t majorityCorrect30 = 0;

    std::size_t actualPositive5 = 0;
    std::size_t actualPositive10 = 0;
    std::size_t actualPositive15 = 0;
    std::size_t actualPositive30 = 0;

    std::size_t naiveCorrect5 = 0;
    std::size_t naiveCorrect10 = 0;
    std::size_t naiveCorrect15 = 0;
    std::size_t naiveCorrect30 = 0;

    /*
        Walk forward through the test set.
    */
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

        std::vector<WeightedMatch>
            matches5;

        std::vector<WeightedMatch>
            matches10;

        std::vector<WeightedMatch>
            matches15;

        std::vector<WeightedMatch>
            matches30;

        /*
            IMPORTANT:

            Do NOT discard the entire sample if
            one horizon cannot build matches.
        */
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

        /*
            PatternX weighted predictions.
        */
        const double prediction5 =
            valid5
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches5,
                    windowSize,
                    5
                  )
                : 0.0;

        const double prediction10 =
            valid10
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches10,
                    windowSize,
                    10
                  )
                : 0.0;

        const double prediction15 =
            valid15
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches15,
                    windowSize,
                    15
                  )
                : 0.0;

        const double prediction30 =
            valid30
                ? calculateWeightedPredictionFromMatches(
                    prices,
                    matches30,
                    windowSize,
                    30
                  )
                : 0.0;

        /*
            Actual test returns.
        */
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

        /*
            ========================================================
            RAW + CALIBRATED CONFIDENCE
            ========================================================

            calculateConfidence() still produces the original raw
            PatternX confidence.

            The calibrated value is then obtained exclusively from
            the training-period calibration table.

            Signal generation below uses calibrated confidence.
        */
        ConfidenceResult confidence5{};
        ConfidenceResult confidence10{};
        ConfidenceResult confidence15{};
        ConfidenceResult confidence30{};

        double rawConfidence5 = 0.0;
        double rawConfidence10 = 0.0;
        double rawConfidence15 = 0.0;
        double rawConfidence30 = 0.0;

        double calibratedConfidence5 = 0.0;
        double calibratedConfidence10 = 0.0;
        double calibratedConfidence15 = 0.0;
        double calibratedConfidence30 = 0.0;

        if (valid5)
        {
            confidence5 =
                calculateConfidence(
                    prices,
                    matches5,
                    windowSize,
                    confidenceThreshold
                );

            rawConfidence5 =
                confidence5.confidence5.confidence;

            calibratedConfidence5 =
                lookupCalibratedConfidence(
                    calibration5,
                    rawConfidence5
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

            rawConfidence10 =
                confidence10.confidence10.confidence;

            calibratedConfidence10 =
                lookupCalibratedConfidence(
                    calibration10,
                    rawConfidence10
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

            rawConfidence15 =
                confidence15.confidence15.confidence;

            calibratedConfidence15 =
                lookupCalibratedConfidence(
                    calibration15,
                    rawConfidence15
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

            rawConfidence30 =
                confidence30.confidence30.confidence;

            calibratedConfidence30 =
                lookupCalibratedConfidence(
                    calibration30,
                    rawConfidence30
                );
        }

        /*
            ========================================================
            PREDICTION AGREEMENT
            ========================================================

            Require at least 3 of the 4 horizons to agree before
            accepting a calibrated signal.

            0.75 is deliberately used as the first, non-tuned
            ensemble agreement threshold because with four horizons
            it means exactly 3/4 directional agreement.
        */
        constexpr double MIN_PREDICTION_AGREEMENT = 0.75;

        const double predictionAgreement =
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

        /*
            calibrated confidence remains the first gate.

            prediction agreement is the second gate.

            Both conditions must be satisfied for a signal.
        */
        if (valid5)
        {
            confidence5.confidence5.signal =
                calibratedConfidence5 >=
                    confidenceThreshold &&
                predictionAgreement >=
                    MIN_PREDICTION_AGREEMENT;
        }

        if (valid10)
        {
            confidence10.confidence10.signal =
                calibratedConfidence10 >=
                    confidenceThreshold &&
                predictionAgreement >=
                    MIN_PREDICTION_AGREEMENT;
        }

        if (valid15)
        {
            confidence15.confidence15.signal =
                calibratedConfidence15 >=
                    confidenceThreshold &&
                predictionAgreement >=
                    MIN_PREDICTION_AGREEMENT;
        }

        if (valid30)
        {
            confidence30.confidence30.signal =
                calibratedConfidence30 >=
                    confidenceThreshold &&
                predictionAgreement >=
                    MIN_PREDICTION_AGREEMENT;
        }

        /*
        ========================================================
        +5 SIGNAL
        ========================================================
        */
        if (valid5 &&
            confidence5.confidence5.signal)
        {
            ++signals5;

            const bool predictedPositive =
                confidence5
                    .confidence5
                    .predictedPositive;

            const double tradeReturn =
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    5,
                    predictedPositive
                );

            totalPnL5 += tradeReturn;

            if (tradeReturn > 0.0)
            {
                ++correctSignals5;
                grossProfit5 += tradeReturn;
            }
            else if (tradeReturn < 0.0)
            {
                grossLoss5 +=
                    std::abs(tradeReturn);
            }

            returnWhenSignaled5 +=
                actual5;
        }

        /*
        ========================================================
        +10 SIGNAL
        ========================================================
        */
        if (valid10 &&
            confidence10.confidence10.signal)
        {
            ++signals10;

            const bool predictedPositive =
                confidence10
                    .confidence10
                    .predictedPositive;

            const double tradeReturn =
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    10,
                    predictedPositive
                );

            totalPnL10 += tradeReturn;

            if (tradeReturn > 0.0)
            {
                ++correctSignals10;
                grossProfit10 += tradeReturn;
            }
            else if (tradeReturn < 0.0)
            {
                grossLoss10 +=
                    std::abs(tradeReturn);
            }

            returnWhenSignaled10 +=
                actual10;
        }

        /*
        ========================================================
        +15 SIGNAL
        ========================================================
        */
        if (valid15 &&
            confidence15.confidence15.signal)
        {
            ++signals15;

            const bool predictedPositive =
                confidence15
                    .confidence15
                    .predictedPositive;

            const double tradeReturn =
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    15,
                    predictedPositive
                );

            totalPnL15 += tradeReturn;

            if (tradeReturn > 0.0)
            {
                ++correctSignals15;
                grossProfit15 += tradeReturn;
            }
            else if (tradeReturn < 0.0)
            {
                grossLoss15 +=
                    std::abs(tradeReturn);
            }

            returnWhenSignaled15 +=
                actual15;
        }

        /*
        ========================================================
        +30 SIGNAL
        ========================================================
        */
        if (valid30 &&
            confidence30.confidence30.signal)
        {
            ++signals30;

            const bool predictedPositive =
                confidence30
                    .confidence30
                    .predictedPositive;

            const double tradeReturn =
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    30,
                    predictedPositive
                );

            totalPnL30 += tradeReturn;

            if (tradeReturn > 0.0)
            {
                ++correctSignals30;
                grossProfit30 += tradeReturn;
            }
            else if (tradeReturn < 0.0)
            {
                grossLoss30 +=
                    std::abs(tradeReturn);
            }

            returnWhenSignaled30 +=
                actual30;
        }

        /*
        ========================================================
        MAJORITY BASELINE
        ========================================================
        */

        majorityPositive5 = 0;
        majorityNegative5 = 0;

        majorityPositive10 = 0;
        majorityNegative10 = 0;

        majorityPositive15 = 0;
        majorityNegative15 = 0;

        majorityPositive30 = 0;
        majorityNegative30 = 0;

        for (std::size_t j = 0;
             j < currentIndex;
             ++j)
        {
            const std::size_t historicalEnd =
                j + windowSize - 1;

            /*
                Complete future must be known before
                the current query begins.
            */
            if (historicalEnd + 30 >=
                currentIndex)
            {
                continue;
            }

            const double r5 =
                calculateReturn(
                    prices,
                    historicalEnd,
                    5
                );

            const double r10 =
                calculateReturn(
                    prices,
                    historicalEnd,
                    10
                );

            const double r15 =
                calculateReturn(
                    prices,
                    historicalEnd,
                    15
                );

            const double r30 =
                calculateReturn(
                    prices,
                    historicalEnd,
                    30
                );

            if (r5 > 0.0)
                ++majorityPositive5;
            else if (r5 < 0.0)
                ++majorityNegative5;

            if (r10 > 0.0)
                ++majorityPositive10;
            else if (r10 < 0.0)
                ++majorityNegative10;

            if (r15 > 0.0)
                ++majorityPositive15;
            else if (r15 < 0.0)
                ++majorityNegative15;

            if (r30 > 0.0)
                ++majorityPositive30;
            else if (r30 < 0.0)
                ++majorityNegative30;
        }

        const bool majorityPrediction5 =
            majorityPositive5 >=
            majorityNegative5;

        const bool majorityPrediction10 =
            majorityPositive10 >=
            majorityNegative10;

        const bool majorityPrediction15 =
            majorityPositive15 >=
            majorityNegative15;

        const bool majorityPrediction30 =
            majorityPositive30 >=
            majorityNegative30;

        if (valid5 &&
            (
                (majorityPrediction5 &&
                 actual5 > 0.0)
                ||
                (!majorityPrediction5 &&
                 actual5 < 0.0)
            ))
        {
            ++majorityCorrect5;
        }

        if (valid10 &&
            (
                (majorityPrediction10 &&
                 actual10 > 0.0)
                ||
                (!majorityPrediction10 &&
                 actual10 < 0.0)
            ))
        {
            ++majorityCorrect10;
        }

        if (valid15 &&
            (
                (majorityPrediction15 &&
                 actual15 > 0.0)
                ||
                (!majorityPrediction15 &&
                 actual15 < 0.0)
            ))
        {
            ++majorityCorrect15;
        }

        if (valid30 &&
            (
                (majorityPrediction30 &&
                 actual30 > 0.0)
                ||
                (!majorityPrediction30 &&
                 actual30 < 0.0)
            ))
        {
            ++majorityCorrect30;
        }

        /*
        ========================================================
        NAIVE MOMENTUM
        ========================================================
        */
        const double trailingReturn =
            calculateTrailingReturn(
                prices,
                currentPriceIndex,
                5
            );

        const bool momentumPositive =
            trailingReturn >= 0.0;

        if (
            valid5 &&
            (
                (momentumPositive &&
                 actual5 > 0.0)
                ||
                (!momentumPositive &&
                 actual5 < 0.0)
            )
        )
        {
            ++naiveCorrect5;
        }

        if (
            valid10 &&
            (
                (momentumPositive &&
                 actual10 > 0.0)
                ||
                (!momentumPositive &&
                 actual10 < 0.0)
            )
        )
        {
            ++naiveCorrect10;
        }

        if (
            valid15 &&
            (
                (momentumPositive &&
                 actual15 > 0.0)
                ||
                (!momentumPositive &&
                 actual15 < 0.0)
            )
        )
        {
            ++naiveCorrect15;
        }

        if (
            valid30 &&
            (
                (momentumPositive &&
                 actual30 > 0.0)
                ||
                (!momentumPositive &&
                 actual30 < 0.0)
            )
        )
        {
            ++naiveCorrect30;
        }

        if (actual5 > 0.0)
            ++actualPositive5;

        if (actual10 > 0.0)
            ++actualPositive10;

        if (actual15 > 0.0)
            ++actualPositive15;

        if (actual30 > 0.0)
            ++actualPositive30;

        /*
        ========================================================
        EQUITY / DRAWDOWN
        ========================================================
        */
        if (valid5 &&
            confidence5.confidence5.signal)
        {
            equity5 +=
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    5,
                    confidence5
                        .confidence5
                        .predictedPositive
                );

            peakEquity5 =
                std::max(
                    peakEquity5,
                    equity5
                );

            maxDrawdown5 =
                std::max(
                    maxDrawdown5,
                    peakEquity5 - equity5
                );
        }

        if (valid10 &&
            confidence10.confidence10.signal)
        {
            equity10 +=
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    10,
                    confidence10
                        .confidence10
                        .predictedPositive
                );

            peakEquity10 =
                std::max(
                    peakEquity10,
                    equity10
                );

            maxDrawdown10 =
                std::max(
                    maxDrawdown10,
                    peakEquity10 - equity10
                );
        }

        if (valid15 &&
            confidence15.confidence15.signal)
        {
            equity15 +=
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    15,
                    confidence15
                        .confidence15
                        .predictedPositive
                );

            peakEquity15 =
                std::max(
                    peakEquity15,
                    equity15
                );

            maxDrawdown15 =
                std::max(
                    maxDrawdown15,
                    peakEquity15 - equity15
                );
        }

        if (valid30 &&
            confidence30.confidence30.signal)
        {
            equity30 +=
                calculateDirectionalTradeReturn(
                    prices,
                    currentPriceIndex,
                    30,
                    confidence30
                        .confidence30
                        .predictedPositive
                );

            peakEquity30 =
                std::max(
                    peakEquity30,
                    equity30
                );

            maxDrawdown30 =
                std::max(
                    maxDrawdown30,
                    peakEquity30 - equity30
                );
        }

        ++metrics.samples;

        /*
        ========================================================
        FIRST FIVE SAMPLES
        ========================================================
        */
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
                    << "% | Confidence: "
                    << rawConfidence5 * 100.0
                    << "% -> Calibrated: "
                    << calibratedConfidence5 * 100.0
                    << "% | "
                    << (
                        confidence5
                            .confidence5
                            .signal
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
                    << "% | Confidence: "
                    << rawConfidence10 * 100.0
                    << "% -> Calibrated: "
                    << calibratedConfidence10 * 100.0
                    << "% | "
                    << (
                        confidence10
                            .confidence10
                            .signal
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
                    << "% | Confidence: "
                    << rawConfidence15 * 100.0
                    << "% -> Calibrated: "
                    << calibratedConfidence15 * 100.0
                    << "% | "
                    << (
                        confidence15
                            .confidence15
                            .signal
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
                    << "% | Confidence: "
                    << rawConfidence30 * 100.0
                    << "% -> Calibrated: "
                    << calibratedConfidence30 * 100.0
                    << "% | "
                    << (
                        confidence30
                            .confidence30
                            .signal
                        ? "SIGNAL"
                        : "NO SIGNAL"
                    )
                    << "\n";
            }
            
            std::cout
                << "Prediction agreement: "
                << predictionAgreement * 100.0
                << "% (minimum 75.00%)\n";
        }
    }

    if (metrics.samples == 0)
    {
        return metrics;
    }

    /*
    ========================================================
    COVERAGE
    ========================================================
    */

    metrics.signals5 = signals5;
    metrics.signals10 = signals10;
    metrics.signals15 = signals15;
    metrics.signals30 = signals30;

    metrics.coverage5 =
        static_cast<double>(signals5) /
        metrics.samples * 100.0;

    metrics.coverage10 =
        static_cast<double>(signals10) /
        metrics.samples * 100.0;

    metrics.coverage15 =
        static_cast<double>(signals15) /
        metrics.samples * 100.0;

    metrics.coverage30 =
        static_cast<double>(signals30) /
        metrics.samples * 100.0;

    /*
    ========================================================
    SIGNAL ACCURACY
    ========================================================
    */

    if (signals5 > 0)
    {
        metrics.signalAccuracy5 =
            static_cast<double>(
                correctSignals5
            ) /
            signals5 * 100.0;

        metrics.averageReturnWhenSignaled5 =
            returnWhenSignaled5 /
            static_cast<double>(signals5);
    }

    if (signals10 > 0)
    {
        metrics.signalAccuracy10 =
            static_cast<double>(
                correctSignals10
            ) /
            signals10 * 100.0;

        metrics.averageReturnWhenSignaled10 =
            returnWhenSignaled10 /
            static_cast<double>(signals10);
    }

    if (signals15 > 0)
    {
        metrics.signalAccuracy15 =
            static_cast<double>(
                correctSignals15
            ) /
            signals15 * 100.0;

        metrics.averageReturnWhenSignaled15 =
            returnWhenSignaled15 /
            static_cast<double>(signals15);
    }

    if (signals30 > 0)
    {
        metrics.signalAccuracy30 =
            static_cast<double>(
                correctSignals30
            ) /
            signals30 * 100.0;

        metrics.averageReturnWhenSignaled30 =
            returnWhenSignaled30 /
            static_cast<double>(signals30);
    }

    /*
    ========================================================
    P&L
    ========================================================
    */

    metrics.totalReturn5 =
        totalPnL5;

    metrics.totalReturn10 =
        totalPnL10;

    metrics.totalReturn15 =
        totalPnL15;

    metrics.totalReturn30 =
        totalPnL30;

    if (signals5 > 0)
    {
        metrics.averageTradeReturn5 =
            totalPnL5 /
            static_cast<double>(signals5);

        metrics.winRate5 =
            static_cast<double>(
                correctSignals5
            ) /
            signals5 * 100.0;
    }

    if (signals10 > 0)
    {
        metrics.averageTradeReturn10 =
            totalPnL10 /
            static_cast<double>(signals10);

        metrics.winRate10 =
            static_cast<double>(
                correctSignals10
            ) /
            signals10 * 100.0;
    }

    if (signals15 > 0)
    {
        metrics.averageTradeReturn15 =
            totalPnL15 /
            static_cast<double>(signals15);

        metrics.winRate15 =
            static_cast<double>(
                correctSignals15
            ) /
            signals15 * 100.0;
    }

    if (signals30 > 0)
    {
        metrics.averageTradeReturn30 =
            totalPnL30 /
            static_cast<double>(signals30);

        metrics.winRate30 =
            static_cast<double>(
                correctSignals30
            ) /
            signals30 * 100.0;
    }

    /*
    ========================================================
    PROFIT FACTOR
    ========================================================
    */

    metrics.profitFactor5 =
        grossLoss5 > 0.0
            ? grossProfit5 / grossLoss5
            : 0.0;

    metrics.profitFactor10 =
        grossLoss10 > 0.0
            ? grossProfit10 / grossLoss10
            : 0.0;

    metrics.profitFactor15 =
        grossLoss15 > 0.0
            ? grossProfit15 / grossLoss15
            : 0.0;

    metrics.profitFactor30 =
        grossLoss30 > 0.0
            ? grossProfit30 / grossLoss30
            : 0.0;

    /*
    ========================================================
    MAX DRAWDOWN
    ========================================================
    */

    metrics.maxDrawdown5 =
        maxDrawdown5;

    metrics.maxDrawdown10 =
        maxDrawdown10;

    metrics.maxDrawdown15 =
        maxDrawdown15;

    metrics.maxDrawdown30 =
        maxDrawdown30;

    /*
    ========================================================
    BASE RATE
    ========================================================
    */

    metrics.baseRatePositive5 =
        static_cast<double>(
            actualPositive5
        ) /
        metrics.samples * 100.0;

    metrics.baseRatePositive10 =
        static_cast<double>(
            actualPositive10
        ) /
        metrics.samples * 100.0;

    metrics.baseRatePositive15 =
        static_cast<double>(
            actualPositive15
        ) /
        metrics.samples * 100.0;

    metrics.baseRatePositive30 =
        static_cast<double>(
            actualPositive30
        ) /
        metrics.samples * 100.0;

    /*
    ========================================================
    NAIVE ACCURACY
    ========================================================
    */

    metrics.naiveAccuracy5 =
        static_cast<double>(
            naiveCorrect5
        ) /
        metrics.samples * 100.0;

    metrics.naiveAccuracy10 =
        static_cast<double>(
            naiveCorrect10
        ) /
        metrics.samples * 100.0;

    metrics.naiveAccuracy15 =
        static_cast<double>(
            naiveCorrect15
        ) /
        metrics.samples * 100.0;

    metrics.naiveAccuracy30 =
        static_cast<double>(
            naiveCorrect30
        ) /
        metrics.samples * 100.0;

    /*
    ========================================================
    MAJORITY ACCURACY
    ========================================================
    */

    metrics.directionalAccuracy5 =
        static_cast<double>(
            majorityCorrect5
        ) /
        metrics.samples * 100.0;

    metrics.directionalAccuracy10 =
        static_cast<double>(
            majorityCorrect10
        ) /
        metrics.samples * 100.0;

    metrics.directionalAccuracy15 =
        static_cast<double>(
            majorityCorrect15
        ) /
        metrics.samples * 100.0;

    metrics.directionalAccuracy30 =
        static_cast<double>(
            majorityCorrect30
        ) /
        metrics.samples * 100.0;

    /*
    ========================================================
    Z-SCORE
    ========================================================
    */

    metrics.zScore5 =
        calculateZScore(
            metrics.signalAccuracy5,
            signals5
        );

    metrics.zScore10 =
        calculateZScore(
            metrics.signalAccuracy10,
            signals10
        );

    metrics.zScore15 =
        calculateZScore(
            metrics.signalAccuracy15,
            signals15
        );

    metrics.zScore30 =
        calculateZScore(
            metrics.signalAccuracy30,
            signals30
        );

    std::cout
        << "\n============================================\n"
        << "CONFIDENCE BACKTEST RESULTS\n"
        << "============================================\n";

    std::cout
        << "Threshold : "
        << confidenceThreshold * 100.0
        << "%\n\n";

    std::cout
        << "## Horizon       Signals       Coverage"
        << "       Accuracy       Avg Return"
        << "       Total P&L       Profit Factor"
        << "       Max Drawdown\n";

    std::cout
        << "+5 days       "
        << signals5
        << "             "
        << metrics.coverage5
        << "%          "
        << metrics.signalAccuracy5
        << "%          "
        << metrics.averageTradeReturn5
        << "%          "
        << metrics.totalReturn5
        << "%          "
        << metrics.profitFactor5
        << "          "
        << metrics.maxDrawdown5
        << "%\n";

    std::cout
        << "+10 days      "
        << signals10
        << "             "
        << metrics.coverage10
        << "%          "
        << metrics.signalAccuracy10
        << "%          "
        << metrics.averageTradeReturn10
        << "%          "
        << metrics.totalReturn10
        << "%          "
        << metrics.profitFactor10
        << "          "
        << metrics.maxDrawdown10
        << "%\n";

    std::cout
        << "+15 days      "
        << signals15
        << "             "
        << metrics.coverage15
        << "%          "
        << metrics.signalAccuracy15
        << "%          "
        << metrics.averageTradeReturn15
        << "%          "
        << metrics.totalReturn15
        << "%          "
        << metrics.profitFactor15
        << "          "
        << metrics.maxDrawdown15
        << "%\n";

    std::cout
        << "+30 days      "
        << signals30
        << "             "
        << metrics.coverage30
        << "%          "
        << metrics.signalAccuracy30
        << "%          "
        << metrics.averageTradeReturn30
        << "%          "
        << metrics.totalReturn30
        << "%          "
        << metrics.profitFactor30
        << "          "
        << metrics.maxDrawdown30
        << "%\n";

    std::cout
        << "\nZ-score +5  : "
        << metrics.zScore5
        << "\n";

    std::cout
        << "Z-score +10 : "
        << metrics.zScore10
        << "\n";

    std::cout
        << "Z-score +15 : "
        << metrics.zScore15
        << "\n";

    std::cout
        << "Z-score +30 : "
        << metrics.zScore30
        << "\n";

    return metrics;
}