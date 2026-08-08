#include <iostream>
#include <vector>

#include "../include/StockData.hpp"

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

    for (size_t i = 0; i < 5 && i < prices.size(); i++){
        std::cout << prices[i] << "\n";
    }


    return 0;
}