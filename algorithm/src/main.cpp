#include <iostream>
#include <vector>

#include "../include/StockData.hpp"
#include "../include/SlidingWindow.hpp"
#include "../include/Normalizer.hpp"

int main()
{
    // Load all stock data
    std::vector<PriceData> data =
        loadStockData("data/stocks.csv");

    std::cout << "Total rows: "
              << data.size()
              << "\n\n";


    // Get TCS data
    std::vector<PriceData> tcs =
        getStock(data, "TCS");

    std::cout << "TCS rows: "
              << tcs.size()
              << "\n\n";


    // Get TCS closing prices
    std::vector<double> prices =
        getClosePrices(data, "TCS");

    std::cout << "TCS closing prices: "
              << prices.size()
              << "\n\n";


    // Print first 5 prices
    std::cout << "First 5 closing prices:\n";

    for (std::size_t i = 0;
         i < 5 && i < prices.size();
         ++i)
    {
        std::cout << prices[i] << "\n";
    }


    // Create sliding windows
    std::size_t windowSize = 30;

    std::vector<std::vector<double>> windows =
        createWindows(prices, windowSize);

    std::cout << "\n";
    std::cout << "Window size: "
              << windowSize
              << "\n";

    std::cout << "Total windows: "
              << windows.size()
              << "\n";


    // Test normalizer
    if (!windows.empty())
    {
        std::cout << "\nFirst window (original):\n";

        for (double price : windows[0])
        {
            std::cout << price << "\n";
        }


        // Normalize first window
        std::vector<double> normalized =
            normalizeWindow(windows[0]);


        std::cout << "\nFirst window (normalized):\n";

        for (double value : normalized)
        {
            std::cout << value << "\n";
        }
    }


    return 0;
}