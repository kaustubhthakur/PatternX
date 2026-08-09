#include "../include/Confidence.hpp"

#include <algorithm>
#include <cstddef>


namespace
{

HorizonConfidence calculateHorizonConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    std::size_t horizon,
    double threshold
)
{
    HorizonConfidence result{};

    if (weightedMatches.empty())
    {
        return result;
    }


    double positiveWeight = 0.0;
    double negativeWeight = 0.0;


    /*
        Look at the future direction of every
        historical matched pattern.

        IMPORTANT:

        This function does NOT use threshold to
        calculate confidence.

        Threshold is only used at the very end
        to determine whether a signal exists.
    */

    for (const auto& match : weightedMatches)
    {
        const std::size_t historicalWindow =
            match.windowIndex;


        const std::size_t endIndex =
            historicalWindow +
            windowSize -
            1;


        const std::size_t futureIndex =
            endIndex +
            horizon;


        /*
            Historical future must exist.
        */

        if (futureIndex >= prices.size())
        {
            continue;
        }


        const double currentPrice =
            prices[endIndex];


        if (currentPrice == 0.0)
        {
            continue;
        }


        const double futurePrice =
            prices[futureIndex];


        const double futureReturn =
            (
                (futurePrice - currentPrice)
                /
                currentPrice
            )
            *
            100.0;


        if (futureReturn > 0.0)
        {
            positiveWeight +=
                match.normalizedWeight;
        }
        else if (futureReturn < 0.0)
        {
            negativeWeight +=
                match.normalizedWeight;
        }

        /*
            Zero return is intentionally ignored.

            It provides no directional information.
        */
    }


    const double totalDirectionalWeight =
        positiveWeight +
        negativeWeight;


    if (totalDirectionalWeight <= 0.0)
    {
        return result;
    }


    /*
        Re-normalize after ignoring zero-return
        historical outcomes or invalid matches.
    */

    positiveWeight /=
        totalDirectionalWeight;

    negativeWeight /=
        totalDirectionalWeight;


    result.positiveWeight =
        positiveWeight;

    result.negativeWeight =
        negativeWeight;


    /*
        Confidence is simply the weighted majority
        direction.

        Example:

        positive = 0.72
        negative = 0.28

        confidence = 0.72
    */

    result.confidence =
        std::max(
            positiveWeight,
            negativeWeight
        );


    result.predictedPositive =
        positiveWeight >=
        negativeWeight;


    /*
        Threshold ONLY controls signal generation.

        It does NOT affect confidence.
    */

    result.signal =
        result.confidence >= threshold;


    return result;
}

}


/*
    Calculate confidence independently for each
    prediction horizon.
*/
ConfidenceResult calculateConfidence(
    const std::vector<double>& prices,
    const std::vector<WeightedMatch>& weightedMatches,
    std::size_t windowSize,
    double threshold
)
{
    ConfidenceResult result{};


    result.confidence5 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            5,
            threshold
        );


    result.confidence10 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            10,
            threshold
        );


    result.confidence15 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            15,
            threshold
        );


    result.confidence30 =
        calculateHorizonConfidence(
            prices,
            weightedMatches,
            windowSize,
            30,
            threshold
        );


    return result;
}