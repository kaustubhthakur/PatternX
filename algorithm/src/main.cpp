#include <iostream>
#include <vector>
#include <complex>

#include "../include/StockData.hpp"
#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"

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

    if (prices.size() < WINDOW_SIZE)
    {
        std::cerr << "Not enough TCS data.\n";
        return 1;
    }

    std::vector<double> window(
        prices.begin(),
        prices.begin() + WINDOW_SIZE
    );

    std::cout << "Window size: "
              << window.size()
              << "\n\n";




    std::cout << "Original 30-day window:\n";

    for (std::size_t i = 0; i < window.size(); ++i)
    {
        std::cout << i
                  << " : "
                  << window[i]
                  << '\n';
    }

    std::cout << "\n";


  

    std::vector<double> normalized =
        normalizeWindow(window);

    std::cout << "Normalized 30-day window:\n";

    for (std::size_t i = 0; i < normalized.size(); ++i)
    {
        std::cout << i
                  << " : "
                  << normalized[i]
                  << '\n';
    }

    std::cout << "\n";


  

    std::cout << "Computing FFT...\n\n";

    std::vector<std::complex<double>> fftResult =
        computeFFT(normalized);


 

    std::vector<double> magnitude =
        computeMagnitude(fftResult);




    std::cout << "FFT size: "
              << fftResult.size()
              << "\n\n";

    std::cout << "FFT Magnitudes:\n";

    for (std::size_t i = 0; i < magnitude.size(); ++i)
    {
        std::cout << "Frequency "
                  << i
                  << " : "
                  << magnitude[i]
                  << '\n';
    }

    std::cout << "\n";




    std::cout << "Complex FFT values:\n";

    for (std::size_t i = 0; i < fftResult.size(); ++i)
    {
        std::cout << "Frequency "
                  << i
                  << " : "
                  << fftResult[i].real()
                  << " + "
                  << fftResult[i].imag()
                  << "i\n";
    }

    std::cout << "\n";

    std::cout << "FFT test completed successfully.\n";

    return 0;
}