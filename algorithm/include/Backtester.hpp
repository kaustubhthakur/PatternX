#ifndef BACKTESTER_HPP
#define BACKTESTER_HPP

#include <cstddef>
#include <vector>



struct BacktestMetrics
{
 
    std::size_t samples = 0;


    double directionalAccuracy5 = 0.0;
    double directionalAccuracy10 = 0.0;
    double directionalAccuracy15 = 0.0;
    double directionalAccuracy30 = 0.0;


    double mae5 = 0.0;
    double mae10 = 0.0;
    double mae15 = 0.0;
    double mae30 = 0.0;


    double baseRatePositive5 = 0.0;
    double baseRatePositive10 = 0.0;
    double baseRatePositive15 = 0.0;
    double baseRatePositive30 = 0.0;


    double naiveAccuracy5 = 0.0;
    double naiveAccuracy10 = 0.0;
    double naiveAccuracy15 = 0.0;
    double naiveAccuracy30 = 0.0;

 
    double majorityAccuracy5 = 0.0;
    double majorityAccuracy10 = 0.0;
    double majorityAccuracy15 = 0.0;
    double majorityAccuracy30 = 0.0;

 
    double zScore5 = 0.0;
    double zScore10 = 0.0;
    double zScore15 = 0.0;
    double zScore30 = 0.0;

    double confidenceThreshold = 0.0;


    std::size_t signals5 = 0;
    std::size_t signals10 = 0;
    std::size_t signals15 = 0;
    std::size_t signals30 = 0;


    double coverage5 = 0.0;
    double coverage10 = 0.0;
    double coverage15 = 0.0;
    double coverage30 = 0.0;


    double signalAccuracy5 = 0.0;
    double signalAccuracy10 = 0.0;
    double signalAccuracy15 = 0.0;
    double signalAccuracy30 = 0.0;


    double averageReturnWhenSignaled5 = 0.0;
    double averageReturnWhenSignaled10 = 0.0;
    double averageReturnWhenSignaled15 = 0.0;
    double averageReturnWhenSignaled30 = 0.0;

 
    double totalReturn5 = 0.0;
    double totalReturn10 = 0.0;
    double totalReturn15 = 0.0;
    double totalReturn30 = 0.0;

   
    double averageTradeReturn5 = 0.0;
    double averageTradeReturn10 = 0.0;
    double averageTradeReturn15 = 0.0;
    double averageTradeReturn30 = 0.0;


    double winRate5 = 0.0;
    double winRate10 = 0.0;
    double winRate15 = 0.0;
    double winRate30 = 0.0;

  
    double profitFactor5 = 0.0;
    double profitFactor10 = 0.0;
    double profitFactor15 = 0.0;
    double profitFactor30 = 0.0;

 
    double maxDrawdown5 = 0.0;
    double maxDrawdown10 = 0.0;
    double maxDrawdown15 = 0.0;
    double maxDrawdown30 = 0.0;

 
    double rawBrierScore5 = 0.0;
    double rawBrierScore10 = 0.0;
    double rawBrierScore15 = 0.0;
    double rawBrierScore30 = 0.0;

    

    double calibratedBrierScore5 = 0.0;
    double calibratedBrierScore10 = 0.0;
    double calibratedBrierScore15 = 0.0;
    double calibratedBrierScore30 = 0.0;


    double rawECE5 = 0.0;
    double rawECE10 = 0.0;
    double rawECE15 = 0.0;
    double rawECE30 = 0.0;

    double calibratedECE5 = 0.0;
    double calibratedECE10 = 0.0;
    double calibratedECE15 = 0.0;
    double calibratedECE30 = 0.0;

    std::size_t calibrationSamples5 = 0;
    std::size_t calibrationSamples10 = 0;
    std::size_t calibrationSamples15 = 0;
    std::size_t calibrationSamples30 = 0;

    

    std::size_t calibrationBuckets5 = 0;
    std::size_t calibrationBuckets10 = 0;
    std::size_t calibrationBuckets15 = 0;
    std::size_t calibrationBuckets30 = 0;

   
    // +5
    std::size_t wins5 = 0;
    std::size_t losses5 = 0;

    double bestTrade5 = 0.0;
    double worstTrade5 = 0.0;

    double averageWin5 = 0.0;
    double averageLoss5 = 0.0;

    double sharpe5 = 0.0;
    double sortino5 = 0.0;
    double calmar5 = 0.0;

    // +10
    std::size_t wins10 = 0;
    std::size_t losses10 = 0;

    double bestTrade10 = 0.0;
    double worstTrade10 = 0.0;

    double averageWin10 = 0.0;
    double averageLoss10 = 0.0;

    double sharpe10 = 0.0;
    double sortino10 = 0.0;
    double calmar10 = 0.0;

    // +15
    std::size_t wins15 = 0;
    std::size_t losses15 = 0;

    double bestTrade15 = 0.0;
    double worstTrade15 = 0.0;

    double averageWin15 = 0.0;
    double averageLoss15 = 0.0;

    double sharpe15 = 0.0;
    double sortino15 = 0.0;
    double calmar15 = 0.0;

    // +30
    std::size_t wins30 = 0;
    std::size_t losses30 = 0;

    double bestTrade30 = 0.0;
    double worstTrade30 = 0.0;

    double averageWin30 = 0.0;
    double averageLoss30 = 0.0;

    double sharpe30 = 0.0;
    double sortino30 = 0.0;
    double calmar30 = 0.0;
};



BacktestMetrics runBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step
);



BacktestMetrics runConfidenceBacktest(
    const std::vector<double>& prices,
    std::size_t windowSize,
    std::size_t topK,
    std::size_t step,
    double trainRatio,
    double confidenceThreshold
);

#endif