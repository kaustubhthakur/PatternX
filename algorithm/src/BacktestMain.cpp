#include <iostream>
#include <vector>
#include <iomanip>

#include "../include/StockData.hpp"
#include "../include/Backtester.hpp"

int main()
{
    std::cout << "============================================\n";
    std::cout << "PATTERNX BACKTEST\n";
    std::cout << "============================================\n\n";

    // Load all stock data
    std::vector<PriceData> data =
        loadStockData("data/stocks.csv");

    std::cout << "Total rows: "
              << data.size()
              << "\n\n";

    // Select TCS
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

    // Backtest configuration
    const std::size_t WINDOW_SIZE = 30;
    const std::size_t TOP_K = 10;
    const std::size_t STEP = 10;

    std::cout << "Window size : "
              << WINDOW_SIZE << "\n";

    std::cout << "Top K       : "
              << TOP_K << "\n";

    std::cout << "Step        : "
              << STEP << "\n\n";

    if (prices.size() < WINDOW_SIZE)
    {
        std::cerr << "Not enough price data.\n";
        return 1;
    }

    // Run historical backtest
    BacktestMetrics metrics =
        runBacktest(
            prices,
            WINDOW_SIZE,
            TOP_K,
            STEP
        );

    // Results
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "BACKTEST RESULTS\n";
    std::cout << "============================================\n\n";

    std::cout << std::fixed
              << std::setprecision(4);

    std::cout << "Samples             : "
              << metrics.samples
              << "\n";

    std::cout << "\n";

   

    std::cout << "MAE +5 days         : "
              << metrics.mae5
              << "%\n";

    std::cout << "MAE +10 days        : "
              << metrics.mae10
              << "%\n";

    std::cout << "MAE +15 days        : "
              << metrics.mae15
              << "%\n";

    std::cout << "MAE +30 days        : "
              << metrics.mae30
              << "%\n";

    std::cout << "\n";



    std::cout << "Directional Accuracy +5 days  : "
              << metrics.directionalAccuracy5
              << "%\n";

    std::cout << "Directional Accuracy +10 days : "
              << metrics.directionalAccuracy10
              << "%\n";

    std::cout << "Directional Accuracy +15 days : "
              << metrics.directionalAccuracy15
              << "%\n";

    std::cout << "Directional Accuracy +30 days : "
              << metrics.directionalAccuracy30
              << "%\n";

 

    std::cout << "\n";
    std::cout << "--------------------------------------------\n";
    std::cout << "MAJORITY VOTE ACCURACY\n";
    std::cout << "--------------------------------------------\n\n";

    std::cout << "Majority Accuracy +5 days  : "
              << metrics.majorityAccuracy5
              << "%\n";

    std::cout << "Majority Accuracy +10 days : "
              << metrics.majorityAccuracy10
              << "%\n";

    std::cout << "Majority Accuracy +15 days : "
              << metrics.majorityAccuracy15
              << "%\n";

    std::cout << "Majority Accuracy +30 days : "
              << metrics.majorityAccuracy30
              << "%\n";


    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "VALIDATION\n";
    std::cout << "============================================\n\n";

    std::cout << "-- Base rate (% of actual returns that were positive) --\n";
    std::cout << "If far from 50%, a model can beat 50% accuracy just by\n";
    std::cout << "guessing the majority sign, without reading any pattern.\n\n";

    std::cout << "Base rate +5 days  : "
              << metrics.baseRatePositive5
              << "%\n";

    std::cout << "Base rate +10 days : "
              << metrics.baseRatePositive10
              << "%\n";

    std::cout << "Base rate +15 days : "
              << metrics.baseRatePositive15
              << "%\n";

    std::cout << "Base rate +30 days : "
              << metrics.baseRatePositive30
              << "%\n";

    std::cout << "\n";

    std::cout << "-- Naive baseline (guess same sign as trailing 5-day return) --\n";
    std::cout << "If this matches or beats the model, FFT matching isn't\n";
    std::cout << "adding value over a much simpler momentum rule.\n\n";

    std::cout << "Naive accuracy +5 days  : "
              << metrics.naiveAccuracy5
              << "%\n";

    std::cout << "Naive accuracy +10 days : "
              << metrics.naiveAccuracy10
              << "%\n";

    std::cout << "Naive accuracy +15 days : "
              << metrics.naiveAccuracy15
              << "%\n";

    std::cout << "Naive accuracy +30 days : "
              << metrics.naiveAccuracy30
              << "%\n";

    std::cout << "\n";

    std::cout << "-- Significance (z-score vs 50% random baseline) --\n";
    std::cout << "|z| > 1.96 is roughly 95% significance IF samples were\n";
    std::cout << "independent. These backtest windows overlap, so treat\n";
    std::cout << "this as an optimistic upper bound, not a guarantee.\n\n";

    std::cout << "Z-score +5 days  : "
              << metrics.zScore5
              << "\n";

    std::cout << "Z-score +10 days : "
              << metrics.zScore10
              << "\n";

    std::cout << "Z-score +15 days : "
              << metrics.zScore15
              << "\n";

    std::cout << "Z-score +30 days : "
              << metrics.zScore30
              << "\n";

    std::cout << "\n";

    std::cout << "============================================\n";
    std::cout << "BACKTEST COMPLETED SUCCESSFULLY\n";
    std::cout << "============================================\n";

    return 0;
}