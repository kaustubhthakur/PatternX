#include "../include/Backtester.hpp"

#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"
#include "../include/PatternMatcher.hpp"
#include "../include/WeightedRanking.hpp"

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
    std::size_t futureIndex =
        startPrice + days;

    if (futureIndex >= prices.size())
    {
        return 0.0;
    }

    double currentPrice =
        prices[startPrice];

    double futurePrice =
        prices[futureIndex];

    if (currentPrice == 0.0)
    {
        return 0.0;
    }

    return ((futurePrice - currentPrice) /
            currentPrice) * 100.0;
}


double calculateWeightedPrediction(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t horizon
)
{
    /*
        Generate FFT signatures and raw price windows.
    */

    std::vector<std::vector<double>> signatures;
    std::vector<std::vector<double>> windows;

    signatures.reserve(currentIndex + 1);
    windows.reserve(currentIndex + 1);

    for (std::size_t start = 0;
         start <= currentIndex;
         ++start)
    {
        std::vector<double> window(
            prices.begin() + start,
            prices.begin() + start + windowSize
        );

        std::vector<double> normalized =
            normalizeWindow(window);

        std::vector<std::complex<double>> fftResult =
            computeFFT(normalized);

        std::vector<double> magnitude =
            computeMagnitude(fftResult);

        signatures.push_back(magnitude);

        // Raw (un-normalized) window, needed for
        // trend-distance comparisons.
        windows.push_back(window);
    }

    const std::vector<double>& currentSignature =
        signatures[currentIndex];

    const std::vector<double>& currentWindow =
        windows[currentIndex];


    /*
        Historical candidates.

        Only use patterns whose future return
        was already known before currentIndex.
    */

    std::vector<std::vector<double>>
        historicalSignatures;

    std::vector<std::vector<double>>
        historicalWindows;

    std::vector<std::size_t>
        historicalIndices;

    for (std::size_t j = 0;
         j < currentIndex;
         ++j)
    {
        std::size_t futureIndex =
            j + windowSize - 1 + horizon;

        /*
            Prevent look-ahead bias.
        */
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

        historicalIndices.push_back(j);
    }

    if (historicalSignatures.empty())
    {
        return 0.0;
    }


    /*
        Find similar historical patterns.

        currentIndex is passed through so that
        findTopMatches can also exclude candidates
        that are within `windowSize` of the query
        window itself, on top of the look-ahead
        filtering already done above. This prevents
        near-duplicate/autocorrelated windows (e.g.
        the window immediately preceding currentIndex)
        from dominating the match set.

        Final argument = minimum separation.

        We use windowSize so that highly overlapping
        historical patterns are not selected together.
    */

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
        return 0.0;
    }


    /*
        Convert local match indices into
        original price-series indices.
    */

    std::vector<std::size_t> windowIndices;
    std::vector<double> distances;

    windowIndices.reserve(matches.size());
    distances.reserve(matches.size());

    for (const auto& match : matches)
    {
        windowIndices.push_back(
            historicalIndices[match.windowIndex]
        );

        distances.push_back(
            match.combinedDistance
        );
    }


    /*
        Distance-based weighting.
    */

    std::vector<WeightedMatch>
        weightedMatches =
            calculateWeights(
                windowIndices,
                distances
            );

    if (weightedMatches.empty())
    {
        return 0.0;
    }


    /*
        Weighted future return prediction.
    */

    double prediction = 0.0;

    for (const auto& match : weightedMatches)
    {
        std::size_t historicalWindow =
            match.windowIndex;

        std::size_t endPriceIndex =
            historicalWindow + windowSize - 1;

        double historicalReturn =
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

} // namespace


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


    /*
        Start far enough into history so that
        candidate patterns have known outcomes.
    */

    const std::size_t MIN_HISTORY =
        windowSize + 30 + 10;


    for (std::size_t currentIndex = MIN_HISTORY;
         currentIndex + 30 < prices.size();
         currentIndex += step)
    {
        /*
            PREDICTIONS
        */

        double prediction5 =
            calculateWeightedPrediction(
                prices,
                currentIndex,
                windowSize,
                topK,
                5
            );

        double prediction10 =
            calculateWeightedPrediction(
                prices,
                currentIndex,
                windowSize,
                topK,
                10
            );

        double prediction15 =
            calculateWeightedPrediction(
                prices,
                currentIndex,
                windowSize,
                topK,
                15
            );

        double prediction30 =
            calculateWeightedPrediction(
                prices,
                currentIndex,
                windowSize,
                topK,
                30
            );


        /*
            ACTUAL FUTURE RETURNS
        */

        std::size_t currentPriceIndex =
            currentIndex + windowSize - 1;

        double actual5 =
            calculateReturn(
                prices,
                currentPriceIndex,
                5
            );

        double actual10 =
            calculateReturn(
                prices,
                currentPriceIndex,
                10
            );

        double actual15 =
            calculateReturn(
                prices,
                currentPriceIndex,
                15
            );

        double actual30 =
            calculateReturn(
                prices,
                currentPriceIndex,
                30
            );


        /*
            MAE
        */

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


        /*
            DIRECTIONAL ACCURACY
        */

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


        ++metrics.samples;


        /*
            Print first 5 samples.
        */

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


    /*
        FINAL MAE
    */

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


    /*
        FINAL DIRECTIONAL ACCURACY
    */

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


    return metrics;
}