
#include <iostream>
#include <vector>
#include <complex>
#include <iomanip>
#include <algorithm>

#include "../include/StockData.hpp"
#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"
#include "../include/PatternMatcher.hpp"

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

    std::cout << "Total windows: "
              << totalWindows
              << "\n\n";



    std::vector<std::vector<double>> signatures;

    signatures.reserve(totalWindows);

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
    }


    std::cout << "FFT signatures generated: "
              << signatures.size()
              << "\n\n";




    const std::size_t currentIndex =
        totalWindows - 1;

    const std::vector<double>& currentSignature =
        signatures[currentIndex];

    std::cout << "Current window index: "
              << currentIndex
              << "\n";

    std::cout << "Current FFT signature size: "
              << currentSignature.size()
              << "\n\n";


   

    std::vector<std::vector<double>> historicalSignatures;

    std::vector<std::size_t> historicalIndices;

    historicalSignatures.reserve(
        totalWindows - 1
    );

    historicalIndices.reserve(
        totalWindows - 1
    );

    for (std::size_t i = 0; i < totalWindows; ++i)
    {
        if (i == currentIndex)
        {
            continue;
        }

        historicalSignatures.push_back(
            signatures[i]
        );

        historicalIndices.push_back(i);
    }



    std::vector<PatternMatch> matches =
        findTopMatches(
            currentSignature,
            historicalSignatures,
            TOP_K
        );



    std::cout << "============================================\n";
    std::cout << "TOP "
              << TOP_K
              << " HISTORICAL PATTERN MATCHES\n";
    std::cout << "============================================\n\n";

    std::cout << std::fixed
              << std::setprecision(6);

    for (std::size_t rank = 0;
         rank < matches.size();
         ++rank)
    {
        const PatternMatch& match =
            matches[rank];

        std::size_t originalWindowIndex =
            historicalIndices[match.windowIndex];

        std::cout << "Rank "
                  << rank + 1
                  << "\n";

        std::cout << "Window index : "
                  << originalWindowIndex
                  << "\n";

        std::cout << "Distance     : "
                  << match.distance
                  << "\n";

        std::cout << "Start day    : "
                  << originalWindowIndex
                  << "\n";

        std::cout << "End day      : "
                  << originalWindowIndex + WINDOW_SIZE - 1
                  << "\n";

        std::cout << "--------------------------------------------\n";
    }




    std::vector<double> currentWindow(
        prices.begin() + currentIndex,
        prices.begin() + currentIndex + WINDOW_SIZE
    );

    std::cout << "\nCurrent 30-day window:\n";

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
    std::cout << "Pattern matching completed successfully.\n";

    return 0;
}

