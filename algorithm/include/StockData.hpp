#ifndef STOCK_DATA_HPP
#define STOCK_DATA_HPP

#include <string>
#include <vector>

struct PriceData {
    std::string symbol;
    std::string date;

    double open;
    double high;
    double low;
    double close;

    long long volume;
};

std::vector<PriceData> loadStockData(
    const std::string& filename
);

std::vector<PriceData> getStock(
    const std::vector<PriceData>& data,
    const std::string& symbol
);

std::vector<double> getClosePrices(
    const std::vector<PriceData>& data,
    const std::string& symbol
);

#endif