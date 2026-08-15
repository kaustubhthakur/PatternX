#include "../include/PatternMatcher.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>


double calculateDistance(
    const std::vector<double>& a,
    const std::vector<double>& b
)
{
    if (a.size() != b.size())
    {
        throw std::invalid_argument(
            "FFT signatures must have the same size"
        );
    }

    double sum = 0.0;

    for (std::size_t i = 0;
         i < a.size();
         ++i)
    {
        double difference =
            a[i] - b[i];

        sum += difference * difference;
    }

    return std::sqrt(sum);
}



double calculateTrendDistance(
    const std::vector<double>& currentWindow,
    const std::vector<double>& historicalWindow
)
{
    if (currentWindow.size() < 2 ||
        historicalWindow.size() < 2)
    {
        return 0.0;
    }

    double currentStart =
        currentWindow.front();

    double currentEnd =
        currentWindow.back();

    double historicalStart =
        historicalWindow.front();

    double historicalEnd =
        historicalWindow.back();

    if (currentStart == 0.0 ||
        historicalStart == 0.0)
    {
        return 0.0;
    }

    double currentTrend =
        (currentEnd - currentStart)
        / currentStart;

    double historicalTrend =
        (historicalEnd - historicalStart)
        / historicalStart;

    return std::abs(
        currentTrend - historicalTrend
    );
}



std::vector<PatternMatch> findTopMatches(
    const std::vector<double>& currentSignature,
    const std::vector<std::vector<double>>& historicalSignatures,
    const std::vector<double>& currentWindow,
    const std::vector<std::vector<double>>& historicalWindows,
    std::size_t currentIndex,
    std::size_t topK,
    std::size_t minSeparation
)
{
    std::vector<PatternMatch> matches;

    if (historicalSignatures.size() !=
        historicalWindows.size())
    {
        throw std::invalid_argument(
            "Historical signatures and windows "
            "must have the same size"
        );
    }

    matches.reserve(
        historicalSignatures.size()
    );

    const double FFT_WEIGHT = 0.70;
    const double TREND_WEIGHT = 0.30;

    for (std::size_t i = 0;
         i < historicalSignatures.size();
         ++i)
    {
        double fftDistance =
            calculateDistance(
                currentSignature,
                historicalSignatures[i]
            );

        double trendDistance =
            calculateTrendDistance(
                currentWindow,
                historicalWindows[i]
            );

        double combinedDistance =
            FFT_WEIGHT * fftDistance +
            TREND_WEIGHT * trendDistance;

        matches.push_back({
            i,
            fftDistance,
            trendDistance,
            combinedDistance
        });
    }

    std::sort(
        matches.begin(),
        matches.end(),
        [](const PatternMatch& a,
           const PatternMatch& b)
        {
            return a.combinedDistance <
                   b.combinedDistance;
        }
    );

    std::vector<PatternMatch> selected;

    selected.reserve(topK);

    for (const auto& candidate : matches)
    {
        
        std::size_t distanceFromCurrent =
            currentIndex > candidate.windowIndex
            ? currentIndex - candidate.windowIndex
            : candidate.windowIndex - currentIndex;

        if (distanceFromCurrent < minSeparation)
        {
            continue;
        }

        
        bool tooClose = false;

        for (const auto& chosen : selected)
        {
            std::size_t difference =
                candidate.windowIndex >
                chosen.windowIndex
                ? candidate.windowIndex -
                  chosen.windowIndex
                : chosen.windowIndex -
                  candidate.windowIndex;

            if (difference < minSeparation)
            {
                tooClose = true;
                break;
            }
        }

        if (!tooClose)
        {
            selected.push_back(candidate);
        }

        if (selected.size() == topK)
        {
            break;
        }
    }

    return selected;
}