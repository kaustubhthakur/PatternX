#include "../include/MultiWindowPrediction.hpp"

#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"
#include "../include/PatternMatcher.hpp"
#include "../include/WeightedRanking.hpp"
#include "../include/RegimeFilter.hpp"

#include <array>
#include <complex>
#include <cstddef>
#include <vector>

namespace
{

constexpr std::array<std::size_t, 4> HORIZONS = {
    5, 10, 15, 30
};


double calculateReturn(
    const std::vector<double>& prices,
    std::size_t startPrice,
    std::size_t days
)
{
    const std::size_t futureIndex =
        startPrice + days;

    if (startPrice >= prices.size() ||
        futureIndex >= prices.size())
    {
        return 0.0;
    }

    const double currentPrice =
        prices[startPrice];

    if (currentPrice == 0.0)
    {
        return 0.0;
    }

    return (
        (prices[futureIndex] - currentPrice) /
        currentPrice
    ) * 100.0;
}


struct WindowPrediction
{
    std::array<double, 4> values{};
    std::array<double, 5> continuationPath{};   // NEW: day 1..5
    bool valid = false;
};


WindowPrediction calculateWindowPrediction(
    const std::vector<double>& prices,
    std::size_t queryStart,
    std::size_t informationCutoff,
    std::size_t windowSize,
    std::size_t topK
)
{
    WindowPrediction result{};

    if (windowSize == 0 ||
        queryStart + windowSize > prices.size())
    {
        return result;
    }


    std::vector<std::vector<double>> signatures;
    std::vector<std::vector<double>> windows;

    signatures.reserve(queryStart + 1);
    windows.reserve(queryStart + 1);


    for (std::size_t start = 0;
         start <= queryStart;
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


        const std::vector<double> normalized =
            normalizeWindow(window);


        const std::vector<std::complex<double>> fftResult =
            computeFFT(normalized);


        const std::vector<double> magnitude =
            computeMagnitude(fftResult);


        signatures.push_back(magnitude);
        windows.push_back(window);
    }


    if (queryStart >= signatures.size())
    {
        return result;
    }


    const std::vector<double>& currentSignature =
        signatures[queryStart];

    const std::vector<double>& currentWindow =
        windows[queryStart];


    std::vector<std::vector<double>> historicalSignatures;
    std::vector<std::vector<double>> historicalWindows;
    std::vector<std::size_t> historicalIndices;

    historicalSignatures.reserve(queryStart);
    historicalWindows.reserve(queryStart);
    historicalIndices.reserve(queryStart);


    for (std::size_t j = 0;
         j < queryStart;
         ++j)
    {
        const std::size_t historicalEnd =
            j + windowSize - 1;


        if (historicalEnd >= prices.size())
        {
            continue;
        }


        const std::size_t historical30End =
            historicalEnd + HORIZONS.back();


        if (historical30End >= informationCutoff)
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
        return result;
    }


    const std::vector<PatternMatch> matches =
        findTopMatches(
            currentSignature,
            historicalSignatures,
            currentWindow,
            historicalWindows,
            queryStart,
            topK,
            windowSize
        );


    if (matches.empty())
    {
        return result;
    }


    std::vector<std::size_t> windowIndices;
    std::vector<double> distances;

    windowIndices.reserve(matches.size());
    distances.reserve(matches.size());


    for (const auto& match : matches)
    {
        if (match.windowIndex >= historicalIndices.size())
        {
            continue;
        }


        windowIndices.push_back(
            historicalIndices[match.windowIndex]
        );

        distances.push_back(
            match.combinedDistance
        );
    }


    if (windowIndices.empty())
    {
        return result;
    }


    const std::vector<WeightedMatch> weightedMatches =
        calculateWeights(
            windowIndices,
            distances
        );


    if (weightedMatches.empty())
    {
        return result;
    }


    const std::size_t currentEndIndex =
        queryStart + windowSize - 1;


    const std::vector<WeightedMatch> regimeFilteredMatches =
        applyRegimeFilter(
            prices,
            currentEndIndex,
            windowSize,
            weightedMatches
        );


    if (regimeFilteredMatches.empty())
    {
        return result;
    }


    /*
        Generate predictions for +5 / +10 / +15 / +30 days.
    */
    for (std::size_t h = 0;
         h < HORIZONS.size();
         ++h)
    {
        double prediction = 0.0;


        for (const auto& match : regimeFilteredMatches)
        {
            const std::size_t endPriceIndex =
                match.windowIndex +
                windowSize -
                1;


            const double historicalReturn =
                calculateReturn(
                    prices,
                    endPriceIndex,
                    HORIZONS[h]
                );


            prediction +=
                match.normalizedWeight *
                historicalReturn;
        }


        result.values[h] = prediction;
    }


    /*
        NEW: Day-by-day continuation path (days 1 through 5).

        Uses the exact same regime-filtered, weighted matches
        already computed above — evaluated at every day instead
        of only the fixed +5/+10/+15/+30 checkpoints. This gives
        the shape of the expected move, not just the endpoint.
    */
    for (std::size_t day = 1; day <= 5; ++day)
    {
        double dayPrediction = 0.0;

        for (const auto& match : regimeFilteredMatches)
        {
            const std::size_t endPriceIndex =
                match.windowIndex +
                windowSize -
                1;

            const double dayReturn =
                calculateReturn(
                    prices,
                    endPriceIndex,
                    day
                );

            dayPrediction +=
                match.normalizedWeight *
                dayReturn;
        }

        result.continuationPath[day - 1] = dayPrediction;
    }


    result.valid = true;

    return result;
}

} // namespace


MultiWindowPrediction calculateMultiWindowPrediction(
    const std::vector<double>& prices,
    std::size_t anchorEndIndex,
    std::size_t topK
)
{
    MultiWindowPrediction result{};


    if (prices.empty() ||
        anchorEndIndex >= prices.size() ||
        topK == 0)
    {
        return result;
    }


    constexpr std::array<std::size_t, 4> windowSizes = {
        15, 30, 60, 90
    };


    const std::size_t informationCutoff =
        anchorEndIndex + 1;


    std::array<double, 4> sums{};
    std::array<double, 5> pathSums{};   // NEW


    for (const std::size_t windowSize : windowSizes)
    {
        if (anchorEndIndex + 1 < windowSize)
        {
            continue;
        }


        const std::size_t queryStart =
            anchorEndIndex + 1 - windowSize;


        const WindowPrediction prediction =
            calculateWindowPrediction(
                prices,
                queryStart,
                informationCutoff,
                windowSize,
                topK
            );


        if (!prediction.valid)
        {
            continue;
        }


        for (std::size_t h = 0;
             h < sums.size();
             ++h)
        {
            sums[h] += prediction.values[h];
        }


        for (std::size_t d = 0;   // NEW
             d < pathSums.size();
             ++d)
        {
            pathSums[d] += prediction.continuationPath[d];
        }


        ++result.validWindowModels;
    }


    if (result.validWindowModels == 0)
    {
        return result;
    }


    const double modelCount =
        static_cast<double>(
            result.validWindowModels
        );


    result.prediction5 =
        sums[0] / modelCount;

    result.prediction10 =
        sums[1] / modelCount;

    result.prediction15 =
        sums[2] / modelCount;

    result.prediction30 =
        sums[3] / modelCount;


    for (std::size_t d = 0;   // NEW
         d < result.continuationPath.size();
         ++d)
    {
        result.continuationPath[d] =
            pathSums[d] / modelCount;
    }


    return result;
}