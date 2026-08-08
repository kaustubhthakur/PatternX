#include <iostream>

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

    if (!tcs.empty()) {

        std::cout << "First TCS record:\n";

        std::cout << "Date: "
                  << tcs[0].date
                  << "\n";

        std::cout << "Close: "
                  << tcs[0].close
                  << "\n";

        std::cout << "Volume: "
                  << tcs[0].volume
                  << "\n";
    }

    return 0;
}