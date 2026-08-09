#include <iostream>
#include <vector>
#include <complex>
#include <iomanip>
#include <cmath>

#include "../include/StockData.hpp"
#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"
#include "../include/PatternMatcher.hpp"
#include "../include/WeightedRanking.hpp"
#include "../include/Prediction.hpp"

int main()
{


    std::vector<PriceData> data =
        loadStockData("data/stocks.csv");

    std::cout << "Total rows: "
              << data.size()
              << "\n\n";




    std::vector<double> prices;

    for (const auto& row : data)
    {
        if (row.symbol == "TCS")
        {
            prices.push_back(row.close);
        }
    }

    std::cout << "TCS closing prices: "
              << prices.size()
              << "\n\n";


    const std::size_t WINDOW_SIZE = 30;
    const std::size_t TOP_K = 10;

    // Prevent highly overlapping historical patterns
    const std::size_t MIN_SEPARATION = WINDOW_SIZE;


    if (prices.size() < WINDOW_SIZE)
    {
        std::cerr << "Not enough TCS data.\n";
        return 1;
    }



    const std::size_t totalWindows =
        prices.size() - WINDOW_SIZE + 1;

    std::cout << "Window size: "
              << WINDOW_SIZE
              << "\n";

    std::cout << "Top K: "
              << TOP_K
              << "\n";

    std::cout << "Minimum separation: "
              << MIN_SEPARATION
              << " days\n";

    std::cout << "Total windows: "
              << totalWindows
              << "\n\n";



    const std::size_t currentIndex =
        totalWindows - 1;



    std::vector<std::vector<double>> signatures;
    std::vector<std::vector<double>> windows;

    signatures.reserve(totalWindows);
    windows.reserve(totalWindows);

    for (std::size_t start = 0;
         start < totalWindows;
         ++start)
    {
        std::vector<double> window(
            prices.begin() + start,
            prices.begin() + start + WINDOW_SIZE
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

    std::cout << "FFT signatures generated: "
              << signatures.size()
              << "\n\n";



    const std::vector<double>& currentSignature =
        signatures[currentIndex];

    const std::vector<double>& currentWindow =
        windows[currentIndex];

    std::cout << "Current window index: "
              << currentIndex
              << "\n";

    std::cout << "Current FFT signature size: "
              << currentSignature.size()
              << "\n\n";



    std::vector<std::vector<double>> historicalSignatures;
    std::vector<std::vector<double>> historicalWindows;
    std::vector<std::size_t> historicalIndices;

    for (std::size_t i = 0;
         i < currentIndex;
         ++i)
    {
        historicalSignatures.push_back(
            signatures[i]
        );

        historicalWindows.push_back(
            windows[i]
        );

        historicalIndices.push_back(i);
    }

    std::cout << "Historical candidates: "
              << historicalSignatures.size()
              << "\n\n";



    std::vector<PatternMatch> matches =
        findTopMatches(
            currentSignature,
            historicalSignatures,
            currentWindow,
            historicalWindows,
            currentIndex,
            TOP_K,
            MIN_SEPARATION
        );


    std::cout << "============================================\n";
    std::cout << "TOP "
              << TOP_K
              << " HISTORICAL PATTERN MATCHES\n";
    std::cout << "============================================\n\n";


    std::cout << std::fixed
              << std::setprecision(6);


    std::vector<std::size_t> windowIndices;
    std::vector<double> distances;

    windowIndices.reserve(matches.size());
    distances.reserve(matches.size());




    for (std::size_t rank = 0;
         rank < matches.size();
         ++rank)
    {
        const PatternMatch& match =
            matches[rank];


        // match.windowIndex already refers to the original
        // window index (it's the raw index passed in via
        // historicalSignatures/historicalWindows, not a
        // local re-indexed position), since those vectors
        // were built in the same order as historicalIndices.
        std::size_t originalIndex =
            historicalIndices[match.windowIndex];


        windowIndices.push_back(
            originalIndex
        );

        distances.push_back(
            match.combinedDistance
        );


        std::cout << "Rank "
                  << rank + 1
                  << "\n";

        std::cout << "Window index : "
                  << originalIndex
                  << "\n";

        std::cout << "Start day    : "
                  << originalIndex
                  << "\n";

        std::cout << "End day      : "
                  << originalIndex + WINDOW_SIZE - 1
                  << "\n";

        std::cout << "FFT distance   : "
                  << match.fftDistance
                  << "\n";

        std::cout << "Trend distance : "
                  << match.trendDistance
                  << "\n";

        std::cout << "Combined dist. : "
                  << match.combinedDistance
                  << "\n";

        std::cout << "--------------------------------------------\n";
    }



    std::vector<WeightedMatch> weightedMatches =
        calculateWeights(
            windowIndices,
            distances
        );


    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "WEIGHTED MATCH RANKING\n";
    std::cout << "============================================\n\n";


    double weightSum = 0.0;


    for (std::size_t rank = 0;
         rank < weightedMatches.size();
         ++rank)
    {
        const WeightedMatch& match =
            weightedMatches[rank];


        weightSum +=
            match.normalizedWeight;


        std::cout << "Rank "
                  << rank + 1
                  << "\n";

        std::cout << "Window index : "
                  << match.windowIndex
                  << "\n";

        std::cout << "Distance     : "
                  << match.distance
                  << "\n";

        std::cout << "Raw weight   : "
                  << match.weight
                  << "\n";

        std::cout << "Normalized   : "
                  << match.normalizedWeight
                  << "\n";

        std::cout << "Weight %     : "
                  << match.normalizedWeight * 100.0
                  << "%\n";

        std::cout << "--------------------------------------------\n";
    }




    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "WEIGHT VERIFICATION\n";
    std::cout << "============================================\n\n";

    std::cout << "Total normalized weight: "
              << weightSum
              << "\n";


    if (std::abs(weightSum - 1.0) < 1e-6)
    {
        std::cout << "Weight test: PASSED\n";
    }
    else
    {
        std::cout << "Weight test: FAILED\n";
    }



    std::vector<std::size_t> predictionIndices;
    std::vector<double> predictionDistances;

    for (const auto& match : matches)
    {
        std::size_t originalIndex =
            historicalIndices[match.windowIndex];


        std::size_t endIndex =
            originalIndex + WINDOW_SIZE - 1;


        // Need at least 30 future trading days
        if (endIndex + 30 < prices.size())
        {
            predictionIndices.push_back(
                originalIndex
            );

            predictionDistances.push_back(
                match.combinedDistance
            );
        }
    }


    std::cout << "\n";
    std::cout << "Prediction candidates: "
              << predictionIndices.size()
              << "\n";



    std::vector<WeightedMatch> predictionMatches =
        calculateWeights(
            predictionIndices,
            predictionDistances
        );


    std::vector<FutureReturns> futureReturns;

    futureReturns.reserve(
        predictionMatches.size()
    );


    std::vector<double> predictionWeights;

    predictionWeights.reserve(
        predictionMatches.size()
    );


    for (const auto& match : predictionMatches)
    {
        FutureReturns returns =
            calculateFutureReturns(
                prices,
                match.windowIndex,
                WINDOW_SIZE
            );


        futureReturns.push_back(
            returns
        );


        predictionWeights.push_back(
            match.normalizedWeight
        );
    }



    PredictionResult prediction =
        calculateWeightedPrediction(
            futureReturns,
            predictionWeights
        );



    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "WEIGHTED FUTURE PREDICTION\n";
    std::cout << "============================================\n\n";


    std::cout << std::fixed
              << std::setprecision(4);


    std::cout << "+5 days\n";

    std::cout << "Expected return : "
              << prediction.prediction5
              << "%\n\n";


    std::cout << "+10 days\n";

    std::cout << "Expected return : "
              << prediction.prediction10
              << "%\n\n";


    std::cout << "+15 days\n";

    std::cout << "Expected return : "
              << prediction.prediction15
              << "%\n\n";


    std::cout << "+30 days\n";

    std::cout << "Expected return : "
              << prediction.prediction30
              << "%\n";



    std::cout << "\n";
    std::cout << "Current "
              << WINDOW_SIZE
              << "-day window:\n";


    for (std::size_t i = 0;
         i < currentWindow.size();
         ++i)
    {
        std::cout << i
                  << " : "
                  << currentWindow[i]
                  << '\n';
    }



    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "PATTERN MATCHING + WEIGHTED PREDICTION\n";
    std::cout << "COMPLETED SUCCESSFULLY\n";
    std::cout << "============================================\n";


    return 0;
}