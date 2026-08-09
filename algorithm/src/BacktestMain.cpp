#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/StockData.hpp"
#include "../include/Backtester.hpp"


void printConfidenceResults(
    const BacktestMetrics& metrics
)
{
    std::cout
        << "\n============================================\n";

    std::cout
        << "CONFIDENCE FILTER RESULTS\n";

    std::cout
        << "============================================\n\n";


    std::cout
        << "Threshold : "
        << metrics.confidenceThreshold * 100.0
        << "%\n\n";


    std::cout
        << "Horizon       Signals       Coverage       "
        << "Accuracy       Avg Return\n";

    std::cout
        << "------------------------------------------------------------\n";


    std::cout
        << "+5 days       "
        << metrics.signals5
        << "             "
        << metrics.coverage5
        << "%          "
        << metrics.signalAccuracy5
        << "%          "
        << metrics.averageReturnWhenSignaled5
        << "%\n";


    std::cout
        << "+10 days      "
        << metrics.signals10
        << "             "
        << metrics.coverage10
        << "%          "
        << metrics.signalAccuracy10
        << "%          "
        << metrics.averageReturnWhenSignaled10
        << "%\n";


    std::cout
        << "+15 days      "
        << metrics.signals15
        << "             "
        << metrics.coverage15
        << "%          "
        << metrics.signalAccuracy15
        << "%          "
        << metrics.averageReturnWhenSignaled15
        << "%\n";


    std::cout
        << "+30 days      "
        << metrics.signals30
        << "             "
        << metrics.coverage30
        << "%          "
        << metrics.signalAccuracy30
        << "%          "
        << metrics.averageReturnWhenSignaled30
        << "%\n";


    std::cout << "\n";


    std::cout
        << "Z-score +5  : "
        << metrics.zScore5
        << "\n";

    std::cout
        << "Z-score +10 : "
        << metrics.zScore10
        << "\n";

    std::cout
        << "Z-score +15 : "
        << metrics.zScore15
        << "\n";

    std::cout
        << "Z-score +30 : "
        << metrics.zScore30
        << "\n";
}


int main()
{
    std::cout
        << "============================================\n";

    std::cout
        << "PATTERNX CONFIDENCE BACKTEST\n";

    std::cout
        << "============================================\n\n";


    /*
        Load all stock data.
    */

    std::vector<PriceData> data =
        loadStockData(
            "data/stocks.csv"
        );


    std::cout
        << "Total rows: "
        << data.size()
        << "\n\n";


    /*
        Select TCS.
    */

    std::vector<double> prices;

    for (const auto& row : data)
    {
        if (row.symbol == "TCS")
        {
            prices.push_back(row.close);
        }
    }


    std::cout
        << "TCS closing prices: "
        << prices.size()
        << "\n\n";


    /*
        Configuration.
    */

    const std::size_t WINDOW_SIZE = 30;

    const std::size_t TOP_K = 10;

    const std::size_t STEP = 10;

    const double TRAIN_RATIO = 0.70;


    std::cout
        << "Window size : "
        << WINDOW_SIZE
        << "\n";

    std::cout
        << "Top K       : "
        << TOP_K
        << "\n";

    std::cout
        << "Step        : "
        << STEP
        << "\n";

    std::cout
        << "Train ratio : "
        << TRAIN_RATIO * 100.0
        << "%\n";


    /*
        Test all thresholds.

        DO NOT pick the best threshold yet.

        We are measuring whether confidence actually
        contains useful information.
    */

    const std::vector<double> thresholds =
    {
        0.60,
        0.70,
        0.80,
        0.90
    };


    std::cout
        << std::fixed
        << std::setprecision(4);


    for (const double threshold : thresholds)
    {
        std::cout
            << "\n\n############################################\n";

        std::cout
            << "CONFIDENCE THRESHOLD: "
            << threshold * 100.0
            << "%\n";

        std::cout
            << "############################################\n";


        BacktestMetrics metrics =
            runConfidenceBacktest(
                prices,
                WINDOW_SIZE,
                TOP_K,
                STEP,
                TRAIN_RATIO,
                threshold
            );


        printConfidenceResults(
            metrics
        );
    }


    std::cout
        << "\n============================================\n";

    std::cout
        << "CONFIDENCE BACKTEST COMPLETED\n";

    std::cout
        << "============================================\n";


    return 0;
}