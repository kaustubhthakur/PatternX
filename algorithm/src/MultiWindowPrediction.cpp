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


    /*
        Build signatures for all windows that can exist
        up to the current query window.
    */
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


    /*
        Select only historical candidates whose complete
        30-day future outcome was already known before
        the current query window begins.

        Historical candidate:

            [j ........ historicalEnd]

        Future outcome:

            historicalEnd + 30

        Requirement:

            historicalEnd + 30 < informationCutoff

        This prevents look-ahead leakage.
    */
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


        /*
            The entire historical pattern outcome must
            be known strictly before the information cutoff.
        */
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


    /*
        Find structurally similar historical patterns.
    */
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


    /*
        Convert PatternMatcher indices into actual
        indices in the original price vector.
    */
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


    /*
        Convert pattern distances into normalized
        similarity weights.
    */
    const std::vector<WeightedMatch> weightedMatches =
        calculateWeights(
            windowIndices,
            distances
        );


    if (weightedMatches.empty())
    {
        return result;
    }


    /*
        ============================================================
        REGIME FILTER
        ============================================================

        PatternMatcher:
            Finds structurally similar historical patterns.

        WeightedRanking:
            Converts pattern distances into weights.

        RegimeFilter:
            Adjusts those weights according to the similarity
            between the historical market regime and the
            current market regime.

        Final weight:

            pattern weight * regime similarity

        The weights are then re-normalized by RegimeFilter.
    */

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
        Generate predictions for:

            +5 days
            +10 days
            +15 days
            +30 days
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


    /*
        Multiple temporal scales.

        15  -> short-term structure
        30  -> medium-term structure
        60  -> larger structure
        90  -> long-term structure
    */
    constexpr std::array<std::size_t, 4> windowSizes = {
        15, 30, 60, 90
    };


    /*
        The query window ends at anchorEndIndex.

        Therefore the first piece of information belonging
        to the future is anchorEndIndex + 1.
    */
    const std::size_t informationCutoff =
        anchorEndIndex + 1;


    std::array<double, 4> sums{};


    for (const std::size_t windowSize : windowSizes)
    {
        /*
            Need at least windowSize observations ending
            at anchorEndIndex.
        */
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


    /*
        Equal-weight ensemble across all valid
        temporal windows.
    */
    result.prediction5 =
        sums[0] / modelCount;

    result.prediction10 =
        sums[1] / modelCount;

    result.prediction15 =
        sums[2] / modelCount;

    result.prediction30 =
        sums[3] / modelCount;


    return result;
}
