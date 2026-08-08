#include "SlidingWindow.hpp"

std::vector<Window> createWindows(
    const std::vector<double>& prices,
    int windowSize
) {
    std::vector<Window> windows;

    int dataSize = prices.size();

    if (dataSize == 0 || windowSize <= 0 || windowSize > dataSize) {
        return windows;
    }

    for (int i = 0; i + windowSize <= dataSize; ++i) {
        windows.push_back({
            i,
            i + windowSize - 1
        });
    }

    return windows;
}