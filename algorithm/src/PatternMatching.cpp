#include "../include/PatternMatcher.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// ------------------------------------------------------------
// Euclidean distance between two FFT signatures
// ------------------------------------------------------------
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

    for (std::size_t i = 0; i < a.size(); ++i)
    {
        double difference = a[i] - b[i];

        sum += difference * difference;
    }

    return std::sqrt(sum);
}


// ------------------------------------------------------------
// Calculate trend distance
//
// We compare the normalized movement from the beginning
// of the window to the end of the window.
//
// Example:
//
// Current:     +10%
// Historical:  +8%
//
// Trend distance = |10 - 8|
//
// This gives us information that FFT alone may miss.
// ------------------------------------------------------------
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

    double currentStart = currentWindow.front();
    double currentEnd   = currentWindow.back();

    double historicalStart = historicalWindow.front();
    double historicalEnd   = historicalWindow.back();

    if (currentStart == 0.0 ||
        historicalStart == 0.0)
    {
        return 0.0;
    }

    // Percentage movement across the window
    double currentTrend =
        (currentEnd - currentStart) / currentStart;

    double historicalTrend =
        (historicalEnd - historicalStart) /
        historicalStart;

    return std::abs(
        currentTrend - historicalTrend
    );
}


// ------------------------------------------------------------
// Find top pattern matches
// ------------------------------------------------------------
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

    // Weight between FFT and trend
    //
    // 70% FFT
    // 30% Trend
    const double FFT_WEIGHT = 0.70;
    const double TREND_WEIGHT = 0.30;

    // --------------------------------------------------------
    // Calculate combined distance
    // --------------------------------------------------------
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


    // --------------------------------------------------------
    // Sort by combined distance
    // --------------------------------------------------------
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


    // --------------------------------------------------------
    // Select separated matches
    //
    // Two separation checks are enforced:
    //
    // 1. Separation from the CURRENT (query) window.
    //    Without this, windows that heavily overlap the
    //    query window (e.g. "yesterday" or "the day before")
    //    will trivially rank as near-perfect matches. This
    //    is autocorrelation, not a genuinely recurring
    //    historical pattern, and it also leaks information:
    //    the "future" outcome of such a window can overlap
    //    with data already inside the current window.
    //
    // 2. Separation between selected matches, so the top-K
    //    set isn't dominated by near-duplicates of each
    //    other.
    // --------------------------------------------------------
    std::vector<PatternMatch> selected;

    selected.reserve(topK);

    for (const auto& candidate : matches)
    {
        // ---- (1) Exclude candidates too close to "now" ----
        std::size_t distanceFromCurrent =
            currentIndex > candidate.windowIndex
            ? currentIndex - candidate.windowIndex
            : candidate.windowIndex - currentIndex;

        if (distanceFromCurrent < minSeparation)
        {
            continue;
        }

        // ---- (2) Exclude candidates too close to matches already chosen ----
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