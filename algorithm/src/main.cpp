#include <iostream>
#include <vector>
#include <complex>

#include "../include/StockData.hpp"
#include "../include/Normalizer.hpp"
#include "../include/FFT.hpp"

int main()
{
    // ============================================
    // 1. Load stock data
    // ============================================

    std::vector<PriceData> data =
        loadStockData("data/stocks.csv");

    std::cout << "Total rows: "
              << data.size()
              << "\n\n";


    // ============================================
    // 2. Extract TCS closing prices
    // ============================================

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


    // ============================================
    // 3. Create first 30-day window
    // ============================================

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


    // ============================================
    // 4. Print original window
    // ============================================

    std::cout << "Original 30-day window:\n";

    for (std::size_t i = 0; i < window.size(); ++i)
    {
        std::cout << i
                  << " : "
                  << window[i]
                  << '\n';
    }

    std::cout << "\n";


    // ============================================
    // 5. Normalize
    // ============================================

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


    // ============================================
    // 6. Compute FFT
    // ============================================

    std::cout << "Computing FFT...\n\n";

    std::vector<std::complex<double>> fftResult =
        computeFFT(normalized);


    // ============================================
    // 7. Compute FFT magnitudes
    // ============================================

    std::vector<double> magnitude =
        computeMagnitude(fftResult);


    // ============================================
    // 8. Print FFT output
    // ============================================

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


    // ============================================
    // 9. Print complex FFT values
    // ============================================

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