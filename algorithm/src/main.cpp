#include <iostream>
#include <vector>

#include "../include/StockData.hpp"
#include "../include/SlidingWindow.hpp"

int main()
{
    
    std::vector<PriceData> data =
        loadStockData("data/stocks.csv");

    std::cout << "Total rows: "
              << data.size()
              << "\n\n";



    std::vector<PriceData> tcs =
        getStock(data, "TCS");

    std::cout << "TCS rows: "
              << tcs.size()
              << "\n\n";


   
    std::vector<double> prices =
        getClosePrices(data, "TCS");

    std::cout << "TCS closing prices: "
              << prices.size()
              << "\n\n";



    std::cout << "First 5 closing prices:\n";

    for (std::size_t i = 0;
         i < 5 && i < prices.size();
         ++i)
    {
        std::cout << prices[i] << "\n";
    }



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


    if (!windows.empty()) {

        std::cout << "\nFirst window:\n";

        for (double price : windows[0]) {
            std::cout << price << "\n";
        }
    }


    return 0;
}