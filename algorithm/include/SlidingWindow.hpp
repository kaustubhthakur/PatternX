#pragma once

#include <vector>

struct Window {
    int startIndex;
    int endIndex;
};

std::vector<Window> createWindows(
    const std::vector<double>& prices,
    int windowSize
);