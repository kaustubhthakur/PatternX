#include "../include/Normalizer.hpp"

std::vector<double> normalizeWindow(
    const std::vector<double>& window
) {
    std::vector<double> normalized;

    if (window.empty()) {
        return normalized;
    }

    double firstPrice = window[0];

    if (firstPrice == 0.0) {
        return normalized;
    }

    normalized.reserve(window.size());

    for (double price : window) {
        double value = (price - firstPrice) / firstPrice;
        normalized.push_back(value);
    }

    return normalized;
}