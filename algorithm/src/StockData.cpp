#include "../include/StockData.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

std::vector<PriceData> loadStockData(
    const std::string& filename
)
{
    std::vector<PriceData> data;

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: "
                  << filename << std::endl;

        return data;
    }

    std::string line;

   
    std::getline(file, line);

    while (std::getline(file, line)) {

        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);

        PriceData stock;

        std::string open;
        std::string high;
        std::string low;
        std::string close;
        std::string volume;

        std::getline(ss, stock.symbol, ',');
        std::getline(ss, stock.date, ',');
        std::getline(ss, open, ',');
        std::getline(ss, high, ',');
        std::getline(ss, low, ',');
        std::getline(ss, close, ',');
        std::getline(ss, volume, ',');

        stock.open = std::stod(open);
        stock.high = std::stod(high);
        stock.low = std::stod(low);
        stock.close = std::stod(close);
        stock.volume = std::stoll(volume);

        data.push_back(stock);
    }

    file.close();

    return data;
}


std::vector<PriceData> getStock(
    const std::vector<PriceData>& data,
    const std::string& symbol
)
{
    std::vector<PriceData> result;

    for (const PriceData& stock : data) {

        if (stock.symbol == symbol) {
            result.push_back(stock);
        }
    }

    return result;
}


std::vector<double> getClosePrices(
    const std::vector<PriceData>& data,
    const std::string& symbol
)
{
    std::vector<double> prices;

    for (const PriceData& stock : data) {

        if (stock.symbol == symbol) {
            prices.push_back(stock.close);
        }
    }

    return prices;
}