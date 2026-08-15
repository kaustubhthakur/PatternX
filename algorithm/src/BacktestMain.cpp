#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/StockData.hpp"
#include "../include/Backtester.hpp"


void printHorizonResults(
    const char* horizon,
    std::size_t signals,
    double coverage,
    double accuracy,
    double avgReturn,
    double totalReturn,
    double profitFactor,
    double maxDrawdown,
    double sharpe,
    double sortino,
    double calmar,
    std::size_t wins,
    std::size_t losses,
    double bestTrade,
    double worstTrade,
    double averageWin,
    double averageLoss
)
{
    std::cout
        << "\n"
        << horizon
        << "\n";

    std::cout
        << "--------------------------------------------\n";

    std::cout
        << "Signals              : "
        << signals
        << "\n";

    std::cout
        << "Coverage             : "
        << coverage
        << "%\n";

    std::cout
        << "Signal Accuracy      : "
        << accuracy
        << "%\n";

    std::cout
        << "Average Trade Return : "
        << avgReturn
        << "%\n";

    std::cout
        << "Total Return         : "
        << totalReturn
        << "%\n";

    std::cout
        << "Wins                 : "
        << wins
        << "\n";

    std::cout
        << "Losses               : "
        << losses
        << "\n";

    std::cout
        << "Average Win          : "
        << averageWin
        << "%\n";

    std::cout
        << "Average Loss         : "
        << averageLoss
        << "%\n";

    std::cout
        << "Best Trade           : "
        << bestTrade
        << "%\n";

    std::cout
        << "Worst Trade          : "
        << worstTrade
        << "%\n";

    std::cout
        << "Profit Factor        : "
        << profitFactor
        << "\n";

    std::cout
        << "Max Drawdown         : "
        << maxDrawdown
        << "%\n";

    std::cout
        << "Sharpe               : "
        << sharpe
        << "\n";

    std::cout
        << "Sortino              : "
        << sortino
        << "\n";

    std::cout
        << "Calmar               : "
        << calmar
        << "\n";
}


void printConfidenceResults(
    const BacktestMetrics& metrics
)
{
    std::cout
        << "\n============================================\n";

    std::cout
        << "CONFIDENCE FILTER RESULTS\n";

    std::cout
        << "============================================\n";

    std::cout
        << "\nThreshold : "
        << metrics.confidenceThreshold * 100.0
        << "%\n";

    printHorizonResults(
        "+5 DAYS",
        metrics.signals5,
        metrics.coverage5,
        metrics.signalAccuracy5,
        metrics.averageTradeReturn5,
        metrics.totalReturn5,
        metrics.profitFactor5,
        metrics.maxDrawdown5,
        metrics.sharpe5,
        metrics.sortino5,
        metrics.calmar5,
        metrics.wins5,
        metrics.losses5,
        metrics.bestTrade5,
        metrics.worstTrade5,
        metrics.averageWin5,
        metrics.averageLoss5
    );

    printHorizonResults(
        "+10 DAYS",
        metrics.signals10,
        metrics.coverage10,
        metrics.signalAccuracy10,
        metrics.averageTradeReturn10,
        metrics.totalReturn10,
        metrics.profitFactor10,
        metrics.maxDrawdown10,
        metrics.sharpe10,
        metrics.sortino10,
        metrics.calmar10,
        metrics.wins10,
        metrics.losses10,
        metrics.bestTrade10,
        metrics.worstTrade10,
        metrics.averageWin10,
        metrics.averageLoss10
    );

    printHorizonResults(
        "+15 DAYS",
        metrics.signals15,
        metrics.coverage15,
        metrics.signalAccuracy15,
        metrics.averageTradeReturn15,
        metrics.totalReturn15,
        metrics.profitFactor15,
        metrics.maxDrawdown15,
        metrics.sharpe15,
        metrics.sortino15,
        metrics.calmar15,
        metrics.wins15,
        metrics.losses15,
        metrics.bestTrade15,
        metrics.worstTrade15,
        metrics.averageWin15,
        metrics.averageLoss15
    );

    printHorizonResults(
        "+30 DAYS",
        metrics.signals30,
        metrics.coverage30,
        metrics.signalAccuracy30,
        metrics.averageTradeReturn30,
        metrics.totalReturn30,
        metrics.profitFactor30,
        metrics.maxDrawdown30,
        metrics.sharpe30,
        metrics.sortino30,
        metrics.calmar30,
        metrics.wins30,
        metrics.losses30,
        metrics.bestTrade30,
        metrics.worstTrade30,
        metrics.averageWin30,
        metrics.averageLoss30
    );

    std::cout
        << "\n--------------------------------------------\n";

    std::cout
        << "BASELINES\n";

    std::cout
        << "--------------------------------------------\n";

    std::cout
        << "Momentum Accuracy +5  : "
        << metrics.naiveAccuracy5
        << "%\n";

    std::cout
        << "Momentum Accuracy +10 : "
        << metrics.naiveAccuracy10
        << "%\n";

    std::cout
        << "Momentum Accuracy +15 : "
        << metrics.naiveAccuracy15
        << "%\n";

    std::cout
        << "Momentum Accuracy +30 : "
        << metrics.naiveAccuracy30
        << "%\n";

    std::cout
        << "\nMajority Accuracy +5  : "
        << metrics.directionalAccuracy5
        << "%\n";

    std::cout
        << "Majority Accuracy +10 : "
        << metrics.directionalAccuracy10
        << "%\n";

    std::cout
        << "Majority Accuracy +15 : "
        << metrics.directionalAccuracy15
        << "%\n";

    std::cout
        << "Majority Accuracy +30 : "
        << metrics.directionalAccuracy30
        << "%\n";

    std::cout
        << "\nZ-score +5  : "
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
        << "PATTERNX CONFIDENCE BACKTEST V2\n";

    std::cout
        << "============================================\n\n";


    std::vector<PriceData> data =
        loadStockData(
            "data/stocks.csv"
        );


    std::cout
        << "Total rows: "
        << data.size()
        << "\n";


    std::vector<double> prices;

    for (const auto& row : data)
    {
        if (row.symbol == "TCS")
        {
            prices.push_back(
                row.close
            );
        }
    }


    std::cout
        << "TCS closing prices: "
        << prices.size()
        << "\n\n";


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


    for (const double threshold :
         thresholds)
    {
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
        << "PATTERNX BACKTEST V2 COMPLETED\n";

    std::cout
        << "============================================\n";


    return 0;
}