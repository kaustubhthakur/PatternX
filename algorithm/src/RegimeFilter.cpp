#include "../include/RegimeFilter.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{

constexpr std::size_t MOMENTUM_LOOKBACK = 5;
constexpr std::size_t TREND_LOOKBACK = 10;
constexpr std::size_t VOLATILITY_LOOKBACK = 10;

/*
    Controls how aggressively regime differences are penalized.

    1.0  = moderate
    >1   = stronger filtering
*/
constexpr double REGIME_STRENGTH = 1.50;

constexpr double EPSILON = 1e-12;


/*
    Return over N periods ending at endIndex.
*/
double calculateReturn(
    const std::vector<double>& prices,
    std::size_t endIndex,
    std::size_t lookback
)
{
    if (endIndex >= prices.size() ||
        endIndex < lookback)
    {
        return 0.0;
    }

    const double oldPrice =
        prices[endIndex - lookback];

    const double currentPrice =
        prices[endIndex];

    if (oldPrice <= EPSILON)
    {
        return 0.0;
    }

    return
        ((currentPrice - oldPrice) /
         oldPrice) * 100.0;
}


/*
    Standard deviation of daily percentage returns.
*/
double calculateVolatility(
    const std::vector<double>& prices,
    std::size_t endIndex,
    std::size_t lookback
)
{
    if (endIndex >= prices.size() ||
        endIndex < lookback)
    {
        return 0.0;
    }

    std::vector<double> returns;
    returns.reserve(lookback);

    for (std::size_t i = 0; i < lookback; ++i)
    {
        const std::size_t current =
            endIndex - i;

        if (current == 0)
        {
            continue;
        }

        const double previousPrice =
            prices[current - 1];

        const double currentPrice =
            prices[current];

        if (previousPrice <= EPSILON)
        {
            continue;
        }

        const double dailyReturn =
            ((currentPrice - previousPrice) /
             previousPrice) * 100.0;

        returns.push_back(dailyReturn);
    }

    if (returns.size() < 2)
    {
        return 0.0;
    }

    const double mean =
        std::accumulate(
            returns.begin(),
            returns.end(),
            0.0
        ) /
        static_cast<double>(returns.size());

    double variance = 0.0;

    for (const double value : returns)
    {
        const double difference =
            value - mean;

        variance +=
            difference * difference;
    }

    variance /=
        static_cast<double>(returns.size());

    return std::sqrt(variance);
}


struct Regime
{
    double momentum5 = 0.0;
    double trend10 = 0.0;
    double volatility10 = 0.0;
};


Regime calculateRegime(
    const std::vector<double>& prices,
    std::size_t endIndex
)
{
    Regime regime{};

    regime.momentum5 =
        calculateReturn(
            prices,
            endIndex,
            MOMENTUM_LOOKBACK
        );

    regime.trend10 =
        calculateReturn(
            prices,
            endIndex,
            TREND_LOOKBACK
        );

    regime.volatility10 =
        calculateVolatility(
            prices,
            endIndex,
            VOLATILITY_LOOKBACK
        );

    return regime;
}


/*
    Convert regime difference into a similarity score.

    Score is in approximately [0, 1]:

        1.0 = very similar regime
        0.0 = very different regime
*/
double calculateRegimeSimilarity(
    const Regime& current,
    const Regime& historical
)
{
    /*
        Return differences.

        The denominator prevents tiny-return regimes
        from becoming excessively sensitive.
    */
    const double momentumDifference =
        std::abs(
            current.momentum5 -
            historical.momentum5
        ) /
        (1.0 +
         std::abs(current.momentum5));

    const double trendDifference =
        std::abs(
            current.trend10 -
            historical.trend10
        ) /
        (2.0 +
         std::abs(current.trend10));

    /*
        Volatility is better compared multiplicatively.

        log ratio:
            0 = identical volatility
            positive/negative = different volatility
    */
    const double currentVolatility =
        std::max(
            current.volatility10,
            EPSILON
        );

    const double historicalVolatility =
        std::max(
            historical.volatility10,
            EPSILON
        );

    const double volatilityDifference =
        std::abs(
            std::log(
                historicalVolatility /
                currentVolatility
            )
        );

  
    const double regimeDistance =
        0.40 * trendDifference +
        0.35 * momentumDifference +
        0.25 * volatilityDifference;

  
    return std::exp(
        -REGIME_STRENGTH *
        regimeDistance
    );
}

} 


std::vector<WeightedMatch> applyRegimeFilter(
    const std::vector<double>& prices,
    std::size_t currentEndIndex,
    std::size_t windowSize,
    const std::vector<WeightedMatch>& weightedMatches
)
{
    if (weightedMatches.empty())
    {
        return {};
    }

    if (prices.empty())
    {
        return weightedMatches;
    }

    if (windowSize == 0 ||
        currentEndIndex >= prices.size())
    {
        return weightedMatches;
    }

  
    const Regime currentRegime =
        calculateRegime(
            prices,
            currentEndIndex
        );

    std::vector<WeightedMatch> result =
        weightedMatches;

    double totalWeight = 0.0;

    for (auto& match : result)
    {
       
        const std::size_t historicalEnd =
            match.windowIndex +
            windowSize -
            1;

        if (historicalEnd >= prices.size())
        {
            match.normalizedWeight = 0.0;
            continue;
        }

      
        const Regime historicalRegime =
            calculateRegime(
                prices,
                historicalEnd
            );

        const double similarity =
            calculateRegimeSimilarity(
                currentRegime,
                historicalRegime
            );

        match.normalizedWeight *=
            similarity;

        totalWeight +=
            match.normalizedWeight;
    }

  
    if (totalWeight <= EPSILON)
    {
        return weightedMatches;
    }

  
    for (auto& match : result)
    {
        match.normalizedWeight /=
            totalWeight;
    }

    return result;
}