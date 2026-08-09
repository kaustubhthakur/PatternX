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
    std::cout << "============================================\n";
    std::cout << "BACKTEST COMPLETED SUCCESSFULLY\n";
    std::cout << "============================================\n";

    return 0;
}