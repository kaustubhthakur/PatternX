#include <iostream>
#include <vector>
#include <complex>
#include <iomanip>
#include <cmath>

#include "../include/StockData.hpp"
#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"
#include "../include/PatternMatcher.hpp"

double calculateFutureReturn(
    const std::vector<double>& prices,
    std::size_t windowIndex,
    std::size_t windowSize,
    std::size_t futureDays
)
{
    // Last price of the historical matched window
    std::size_t endIndex =
        windowIndex + windowSize - 1;

    // Price after futureDays
    std::size_t futureIndex =
        endIndex + futureDays;

    if (futureIndex >= prices.size())
    {
        return 0.0;
    }

    double currentPrice = prices[endIndex];
    double futurePrice = prices[futureIndex];

    return ((futurePrice - currentPrice) / currentPrice) * 100.0;
}


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

    const std::size_t HORIZON_5 = 5;
    const std::size_t HORIZON_10 = 10;
    const std::size_t HORIZON_15 = 15;
    const std::size_t HORIZON_30 = 30;

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




    const std::size_t currentIndex =
        totalWindows - 1;


   

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

        // Normalize
        std::vector<double> normalized =
            normalizeWindow(window);

        // FFT
        std::vector<std::complex<double>> fftResult =
            computeFFT(normalized);

        // Magnitude
        std::vector<double> magnitude =
            computeMagnitude(fftResult);

        signatures.push_back(magnitude);
    }

    std::cout << "FFT signatures generated: "
              << signatures.size()
              << "\n\n";




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

    for (std::size_t i = 0;
         i < currentIndex;
         ++i)
    {
        std::size_t windowEnd =
            i + WINDOW_SIZE - 1;

        std::size_t future30Index =
            windowEnd + HORIZON_30;

     
        if (future30Index >= prices.size())
        {
            continue;
        }

        historicalSignatures.push_back(
            signatures[i]
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
            TOP_K
        );


    std::cout << "============================================\n";
    std::cout << "TOP "
              << TOP_K
              << " HISTORICAL PATTERN MATCHES\n";
    std::cout << "============================================\n\n";

    std::cout << std::fixed
              << std::setprecision(4);


    double totalReturn5 = 0.0;
    double totalReturn10 = 0.0;
    double totalReturn15 = 0.0;
    double totalReturn30 = 0.0;

    for (std::size_t rank = 0;
         rank < matches.size();
         ++rank)
    {
        const PatternMatch& match =
            matches[rank];

       
        std::size_t originalWindowIndex =
            historicalIndices[match.windowIndex];


       
        double return5 =
            calculateFutureReturn(
                prices,
                originalWindowIndex,
                WINDOW_SIZE,
                HORIZON_5
            );

        double return10 =
            calculateFutureReturn(
                prices,
                originalWindowIndex,
                WINDOW_SIZE,
                HORIZON_10
            );

        double return15 =
            calculateFutureReturn(
                prices,
                originalWindowIndex,
                WINDOW_SIZE,
                HORIZON_15
            );

        double return30 =
            calculateFutureReturn(
                prices,
                originalWindowIndex,
                WINDOW_SIZE,
                HORIZON_30
            );


        totalReturn5 += return5;
        totalReturn10 += return10;
        totalReturn15 += return15;
        totalReturn30 += return30;




        std::cout << "Rank "
                  << rank + 1
                  << "\n";

        std::cout << "Window index : "
                  << originalWindowIndex
                  << "\n";

        std::cout << "Start day    : "
                  << originalWindowIndex
                  << "\n";

        std::cout << "End day      : "
                  << originalWindowIndex + WINDOW_SIZE - 1
                  << "\n";

        std::cout << "Distance     : "
                  << match.distance
                  << "\n";


    

        std::size_t endIndex =
            originalWindowIndex + WINDOW_SIZE - 1;

        std::cout << "End price    : ₹"
                  << prices[endIndex]
                  << "\n";



        std::cout << "\nFuture Returns:\n";

        std::cout << "+5 days      : "
                  << return5
                  << "%\n";

        std::cout << "+10 days     : "
                  << return10
                  << "%\n";

        std::cout << "+15 days     : "
                  << return15
                  << "%\n";

        std::cout << "+30 days     : "
                  << return30
                  << "%\n";

        std::cout << "--------------------------------------------\n";
    }



    if (!matches.empty())
    {
        double count =
            static_cast<double>(matches.size());

        double average5 =
            totalReturn5 / count;

        double average10 =
            totalReturn10 / count;

        double average15 =
            totalReturn15 / count;

        double average30 =
            totalReturn30 / count;


        std::cout << "\n";
        std::cout << "============================================\n";
        std::cout << "AVERAGE FUTURE RETURNS\n";
        std::cout << "============================================\n";

        std::cout << "+5 days      : "
                  << average5
                  << "%\n";

        std::cout << "+10 days     : "
                  << average10
                  << "%\n";

        std::cout << "+15 days     : "
                  << average15
                  << "%\n";

        std::cout << "+30 days     : "
                  << average30
                  << "%\n";
    }



    std::vector<double> currentWindow(
        prices.begin() + currentIndex,
        prices.begin() + currentIndex + WINDOW_SIZE
    );

    std::cout << "\n";
    std::cout << "Current 30-day window:\n";

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
    std::cout << "Pattern matching + future analysis "
              << "completed successfully.\n";

    return 0;
}