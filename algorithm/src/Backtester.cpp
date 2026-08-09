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

    std::size_t pastIndex =
        priceIndex - days;

    double pastPrice =
        prices[pastIndex];

    double currentPrice =
        prices[priceIndex];

    if (pastPrice == 0.0)
    {
        return 0.0;
    }

    return ((currentPrice - pastPrice) /
            pastPrice) * 100.0;
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

    double observed =
        accuracyPercent / 100.0;

    double standardError =
        std::sqrt(0.25 / static_cast<double>(n));

    if (standardError == 0.0)
    {
        return 0.0;
    }

    return (observed - 0.5) / standardError;
}


double calculateWeightedPrediction(
    const std::vector<double>& prices,
    std::size_t currentIndex,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t horizon
)
{
  

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

    
        windows.push_back(window);
    }

    const std::vector<double>& currentSignature =
        signatures[currentIndex];

    const std::vector<double>& currentWindow =
        windows[currentIndex];


 

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


    

        double trailingReturn =
            calculateTrailingReturn(
                prices,
                currentPriceIndex,
                5
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



        if (actual5 >= 0.0)  ++actualPositive5;
        if (actual10 >= 0.0) ++actualPositive10;
        if (actual15 >= 0.0) ++actualPositive15;
        if (actual30 >= 0.0) ++actualPositive30;


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


    /*
        SIGNIFICANCE (z-scores vs 50% random baseline)
    */

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