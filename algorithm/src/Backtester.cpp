#include "../include/Backtester.hpp"

#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"
#include "../include/PatternMatcher.hpp"
#include "../include/WeightedRanking.hpp"
#include "../include/Confidence.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
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
    Build the pattern universe available at a
    particular query index.

    A historical window is allowed only if its
    entire future outcome is already known before
    the query begins.
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

    /*
        Only construct windows that can actually
        fit inside the known data.
    */
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

        Historical pattern's complete future outcome
        must exist before currentIndex.

        Example:

        query starts at 1703

        historical window:
        start + windowSize - 1 + horizon

        must be < 1703
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
        windowIndices.push_back(
            candidateIndices[
                match.windowIndex
            ]
        );

        distances.push_back(
            match.combinedDistance
        );
    }


    weightedMatches =
        calculateWeights(
            windowIndices,
            distances
        );

    return !weightedMatches.empty();
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


        buildMatches(
            prices,
            currentIndex,
            windowSize,
            topK,
            5,
            matches5
        );

        buildMatches(
            prices,
            currentIndex,
            windowSize,
            topK,
            10,
            matches10
        );

        buildMatches(
            prices,
            currentIndex,
            windowSize,
            topK,
            15,
            matches15
        );

        buildMatches(
            prices,
            currentIndex,
            windowSize,
            topK,
            30,
            matches30
        );


        double prediction5 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches5,
                windowSize,
                5
            );

        double prediction10 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches10,
                windowSize,
                10
            );

        double prediction15 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches15,
                windowSize,
                15
            );

        double prediction30 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches30,
                windowSize,
                30
            );


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


        const double trailingReturn =
            calculateTrailingReturn(
                prices,
                currentPriceIndex,
                5
            );


        error5 +=
            std::abs(
                prediction5 - actual5
            );

        error10 +=
            std::abs(
                prediction10 - actual10
            );

        error15 +=
            std::abs(
                prediction15 - actual15
            );

        error30 +=
            std::abs(
                prediction30 - actual30
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


        if (
            (prediction10 >= 0.0 &&
             actual10 >= 0.0) ||
            (prediction10 < 0.0 &&
             actual10 < 0.0)
        )
        {
            ++correct10;
        }


        if (
            (prediction15 >= 0.0 &&
             actual15 >= 0.0) ||
            (prediction15 < 0.0 &&
             actual15 < 0.0)
        )
        {
            ++correct15;
        }


        if (
            (prediction30 >= 0.0 &&
             actual30 >= 0.0) ||
            (prediction30 < 0.0 &&
             actual30 < 0.0)
        )
        {
            ++correct30;
        }


        if (
            (trailingReturn >= 0.0 &&
             actual5 >= 0.0) ||
            (trailingReturn < 0.0 &&
             actual5 < 0.0)
        )
        {
            ++naiveCorrect5;
        }


        if (
            (trailingReturn >= 0.0 &&
             actual10 >= 0.0) ||
            (trailingReturn < 0.0 &&
             actual10 < 0.0)
        )
        {
            ++naiveCorrect10;
        }


        if (
            (trailingReturn >= 0.0 &&
             actual15 >= 0.0) ||
            (trailingReturn < 0.0 &&
             actual15 < 0.0)
        )
        {
            ++naiveCorrect15;
        }


        if (
            (trailingReturn >= 0.0 &&
             actual30 >= 0.0) ||
            (trailingReturn < 0.0 &&
             actual30 < 0.0)
        )
        {
            ++naiveCorrect30;
        }


        if (actual5 >= 0.0)
            ++actualPositive5;

        if (actual10 >= 0.0)
            ++actualPositive10;

        if (actual15 >= 0.0)
            ++actualPositive15;

        if (actual30 >= 0.0)
            ++actualPositive30;


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

        The first test query must have its entire
        30-day window inside the training boundary.
    */

    const std::size_t initialTrainEnd =
        static_cast<std::size_t>(
            static_cast<double>(prices.size())
            * trainRatio
        );


    /*
        We need the query window itself to fit
        before we reach the initial training boundary.
    */

    if (initialTrainEnd < windowSize)
    {
        return metrics;
    }


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
        /*
            Need the entire query window and
            future horizon available.
        */

        const std::size_t
            currentPriceIndex =
                currentIndex +
                windowSize - 1;

        if (currentPriceIndex + 30 >=
            prices.size())
        {
            break;
        }


        /*
            Calculate PatternX separately for each
            horizon because a historical pattern is
            usable only when its corresponding future
            outcome was known before the query.
        */

        std::vector<WeightedMatch>
            matches5;

        std::vector<WeightedMatch>
            matches10;

        std::vector<WeightedMatch>
            matches15;

        std::vector<WeightedMatch>
            matches30;


        if (!buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                5,
                matches5))
        {
            continue;
        }


        if (!buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                10,
                matches10))
        {
            continue;
        }


        if (!buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                15,
                matches15))
        {
            continue;
        }


        if (!buildMatches(
                prices,
                currentIndex,
                windowSize,
                topK,
                30,
                matches30))
        {
            continue;
        }


        /*
            PatternX weighted predictions.
        */

        const double prediction5 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches5,
                windowSize,
                5
            );

        const double prediction10 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches10,
                windowSize,
                10
            );

        const double prediction15 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches15,
                windowSize,
                15
            );

        const double prediction30 =
            calculateWeightedPredictionFromMatches(
                prices,
                matches30,
                windowSize,
                30
            );


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
            Confidence.

            IMPORTANT:
            These confidence values are calculated
            ONLY from historical matches available
            at this test point.
        */

        const ConfidenceResult confidence5 =
            calculateConfidence(
                prices,
                matches5,
                windowSize,
                confidenceThreshold
            );

        const ConfidenceResult confidence10 =
            calculateConfidence(
                prices,
                matches10,
                windowSize,
                confidenceThreshold
            );

        const ConfidenceResult confidence15 =
            calculateConfidence(
                prices,
                matches15,
                windowSize,
                confidenceThreshold
            );

        const ConfidenceResult confidence30 =
            calculateConfidence(
                prices,
                matches30,
                windowSize,
                confidenceThreshold
            );


        /*
            Confidence signals.
        */

        if (confidence5.confidence5.signal)
        {
            ++signals5;

            if (
                (confidence5.confidence5.predictedPositive &&
                 actual5 > 0.0)
                ||
                (!confidence5.confidence5.predictedPositive &&
                 actual5 < 0.0)
            )
            {
                ++correctSignals5;
            }

            returnWhenSignaled5 += actual5;
        }


        if (confidence10.confidence10.signal)
        {
            ++signals10;

            if (
                (confidence10.confidence10.predictedPositive &&
                 actual10 > 0.0)
                ||
                (!confidence10.confidence10.predictedPositive &&
                 actual10 < 0.0)
            )
            {
                ++correctSignals10;
            }

            returnWhenSignaled10 += actual10;
        }


        if (confidence15.confidence15.signal)
        {
            ++signals15;

            if (
                (confidence15.confidence15.predictedPositive &&
                 actual15 > 0.0)
                ||
                (!confidence15.confidence15.predictedPositive &&
                 actual15 < 0.0)
            )
            {
                ++correctSignals15;
            }

            returnWhenSignaled15 += actual15;
        }


        if (confidence30.confidence30.signal)
        {
            ++signals30;

            if (
                (confidence30.confidence30.predictedPositive &&
                 actual30 > 0.0)
                ||
                (!confidence30.confidence30.predictedPositive &&
                 actual30 < 0.0)
            )
            {
                ++correctSignals30;
            }

            returnWhenSignaled30 += actual30;
        }


        /*
            Training-only majority baseline.

            Training data ends at currentIndex.

            We deliberately don't use any future test
            information here.
        */

        majorityPositive5 = 0;
        majorityNegative5 = 0;

        majorityPositive10 = 0;
        majorityNegative10 = 0;

        majorityPositive15 = 0;
        majorityNegative15 = 0;

        majorityPositive30 = 0;
        majorityNegative30 = 0;


        /*
            Only count historical outcomes whose
            complete future is before currentIndex.
        */

        for (std::size_t j = 0;
             j < currentIndex;
             ++j)
        {
            const std::size_t
                historicalEnd =
                    j + windowSize - 1;


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


        if (
            (majorityPrediction5 &&
             actual5 > 0.0)
            ||
            (!majorityPrediction5 &&
             actual5 < 0.0)
        )
        {
            ++majorityCorrect5;
        }


        if (
            (majorityPrediction10 &&
             actual10 > 0.0)
            ||
            (!majorityPrediction10 &&
             actual10 < 0.0)
        )
        {
            ++majorityCorrect10;
        }


        if (
            (majorityPrediction15 &&
             actual15 > 0.0)
            ||
            (!majorityPrediction15 &&
             actual15 < 0.0)
        )
        {
            ++majorityCorrect15;
        }


        if (
            (majorityPrediction30 &&
             actual30 > 0.0)
            ||
            (!majorityPrediction30 &&
             actual30 < 0.0)
        )
        {
            ++majorityCorrect30;
        }


        /*
            Naive trailing 5-day momentum.
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
            (momentumPositive &&
             actual5 > 0.0)
            ||
            (!momentumPositive &&
             actual5 < 0.0)
        )
        {
            ++naiveCorrect5;
        }


        if (
            (momentumPositive &&
             actual10 > 0.0)
            ||
            (!momentumPositive &&
             actual10 < 0.0)
        )
        {
            ++naiveCorrect10;
        }


        if (
            (momentumPositive &&
             actual15 > 0.0)
            ||
            (!momentumPositive &&
             actual15 < 0.0)
        )
        {
            ++naiveCorrect15;
        }


        if (
            (momentumPositive &&
             actual30 > 0.0)
            ||
            (!momentumPositive &&
             actual30 < 0.0)
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


        ++metrics.samples;


        /*
            Print first five examples.
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
                << "+5  Predicted: "
                << prediction5
                << "% | Actual: "
                << actual5
                << "% | Confidence: "
                << confidence5.confidence5.confidence * 100.0
                << "% | "
                << (
                    confidence5.confidence5.signal
                    ? "SIGNAL"
                    : "NO SIGNAL"
                )
                << "\n";

            std::cout
                << "+10 Predicted: "
                << prediction10
                << "% | Actual: "
                << actual10
                << "% | Confidence: "
                << confidence10.confidence10.confidence * 100.0
                << "% | "
                << (
                    confidence10.confidence10.signal
                    ? "SIGNAL"
                    : "NO SIGNAL"
                )
                << "\n";

            std::cout
                << "+15 Predicted: "
                << prediction15
                << "% | Actual: "
                << actual15
                << "% | Confidence: "
                << confidence15.confidence15.confidence * 100.0
                << "% | "
                << (
                    confidence15.confidence15.signal
                    ? "SIGNAL"
                    : "NO SIGNAL"
                )
                << "\n";

            std::cout
                << "+30 Predicted: "
                << prediction30
                << "% | Actual: "
                << actual30
                << "% | Confidence: "
                << confidence30.confidence30.confidence * 100.0
                << "% | "
                << (
                    confidence30.confidence30.signal
                    ? "SIGNAL"
                    : "NO SIGNAL"
                )
                << "\n";
        }
    }


    if (metrics.samples == 0)
    {
        return metrics;
    }


    /*
        Coverage.
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
        Accuracy when a signal actually exists.
    */

    if (signals5 > 0)
    {
        metrics.signalAccuracy5 =
            static_cast<double>(correctSignals5) /
            signals5 * 100.0;

        metrics.averageReturnWhenSignaled5 =
            returnWhenSignaled5 /
            static_cast<double>(signals5);
    }


    if (signals10 > 0)
    {
        metrics.signalAccuracy10 =
            static_cast<double>(correctSignals10) /
            signals10 * 100.0;

        metrics.averageReturnWhenSignaled10 =
            returnWhenSignaled10 /
            static_cast<double>(signals10);
    }


    if (signals15 > 0)
    {
        metrics.signalAccuracy15 =
            static_cast<double>(correctSignals15) /
            signals15 * 100.0;

        metrics.averageReturnWhenSignaled15 =
            returnWhenSignaled15 /
            static_cast<double>(signals15);
    }


    if (signals30 > 0)
    {
        metrics.signalAccuracy30 =
            static_cast<double>(correctSignals30) /
            signals30 * 100.0;

        metrics.averageReturnWhenSignaled30 =
            returnWhenSignaled30 /
            static_cast<double>(signals30);
    }


    /*
        PatternX full-test directional accuracy.

        These remain included so we can compare:

        PatternX vs confidence-filtered PatternX.
    */

    /*
        For this function we calculate these from
        the signal direction only when available.
        The actual prediction direction is still
        represented by the weighted prediction.
    */

    /*
        Training-only majority.
    */

    metrics.baseRatePositive5 =
        static_cast<double>(actualPositive5) /
        metrics.samples * 100.0;

    metrics.baseRatePositive10 =
        static_cast<double>(actualPositive10) /
        metrics.samples * 100.0;

    metrics.baseRatePositive15 =
        static_cast<double>(actualPositive15) /
        metrics.samples * 100.0;

    metrics.baseRatePositive30 =
        static_cast<double>(actualPositive30) /
        metrics.samples * 100.0;


    metrics.naiveAccuracy5 =
        static_cast<double>(naiveCorrect5) /
        metrics.samples * 100.0;

    metrics.naiveAccuracy10 =
        static_cast<double>(naiveCorrect10) /
        metrics.samples * 100.0;

    metrics.naiveAccuracy15 =
        static_cast<double>(naiveCorrect15) /
        metrics.samples * 100.0;

    metrics.naiveAccuracy30 =
        static_cast<double>(naiveCorrect30) /
        metrics.samples * 100.0;


    /*
        Majority accuracy.
    */

    metrics.directionalAccuracy5 =
        static_cast<double>(majorityCorrect5) /
        metrics.samples * 100.0;

    metrics.directionalAccuracy10 =
        static_cast<double>(majorityCorrect10) /
        metrics.samples * 100.0;

    metrics.directionalAccuracy15 =
        static_cast<double>(majorityCorrect15) /
        metrics.samples * 100.0;

    metrics.directionalAccuracy30 =
        static_cast<double>(majorityCorrect30) /
        metrics.samples * 100.0;


    /*
        Z-score for the confidence signals.

        Here we use the number of actual signals,
        not the entire test set.
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


    return metrics;
}