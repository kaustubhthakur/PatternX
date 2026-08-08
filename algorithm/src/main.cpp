#include <iostream>
#include <vector>
#include "SlidingWindow.hpp"

using namespace std;

int main() {

    vector<double> prices = {
        100,
        105,
        103,
        108,
        110,
        107,
        112,
        115,
        113,
        118
    };

    int windowSize = 3;

    vector<Window> windows =
        createWindows(prices, windowSize);

    cout << "Total data points: "
         << prices.size() << "\n";

    cout << "Window size: "
         << windowSize << "\n";

    cout << "Total windows: "
         << windows.size() << "\n\n";

    for (const Window& window : windows) {

        cout << "Start: "
             << window.startIndex

             << " | End: "
             << window.endIndex

             << "\n";
    }

    return 0;
}