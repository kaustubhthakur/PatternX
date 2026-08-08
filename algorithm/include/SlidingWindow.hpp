#ifndef SLIDING_WINDOW_HPP
#define SLIDING_WINDOW_HPP

#include <cstddef>
#include <vector>

std::vector<std::vector<double>> createWindows(
    const std::vector<double>& prices,
    std::size_t windowSize
);

#endif