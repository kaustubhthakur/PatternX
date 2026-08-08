#include "../include/SlidingWindow.hpp"

std::vector<std::vector<double>> createWindows(
    const std::vector<double>& prices,
    std::size_t windowSize
)
{
    std::vector<std::vector<double>> windows;

    // Invalid window
    if (windowSize == 0 || windowSize > prices.size()) {
        return windows;
    }

    std::size_t totalWindows =
        prices.size() - windowSize + 1;

    windows.reserve(totalWindows);

    for (std::size_t start = 0;
         start < totalWindows;
         ++start)
    {
        std::vector<double> window;

        window.reserve(windowSize);

        for (std::size_t i = 0;
             i < windowSize;
             ++i)
        {
            window.push_back(prices[start + i]);
        }

        windows.push_back(window);
    }

    return windows;
}