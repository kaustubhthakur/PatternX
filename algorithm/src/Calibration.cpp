#include "../include/Calibration.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

/*
    Builds an empirical calibration table from training-period
    (rawConfidence, wasCorrect) observations.

    Points are sorted by raw confidence, split into `numBuckets`
    equal-sized groups, and each bucket's empirical accuracy is
    computed as the fraction of correct predictions within it.

    A simplified isotonic-regression pass then merges any
    adjacent buckets that violate monotonicity (a later bucket
    scoring lower accuracy than an earlier one), guaranteeing the
    final table is non-decreasing in raw confidence. This is what
    makes lookupCalibratedConfidence() safe to use directly as a
    threshold-comparable confidence score.

    Returns an empty table if there isn't enough data to form
    reliable buckets (at least numBuckets * 3 points required) —
    callers should fall back to the raw confidence score in that
    case rather than trust a table built from too little data.
*/
std::vector<CalibrationBucket> buildCalibrationTable(
    std::vector<CalibrationPoint> points,
    std::size_t numBuckets
)
{
    std::vector<CalibrationBucket> table;

    if (numBuckets == 0)
    {
        return table;
    }

    if (points.size() < numBuckets * 3)
    {
        // Not enough training observations to trust bucketed
        // statistics — caller should fall back to raw confidence.
        return table;
    }

    std::sort(
        points.begin(),
        points.end(),
        [](const CalibrationPoint& a, const CalibrationPoint& b)
        {
            return a.rawConfidence < b.rawConfidence;
        }
    );

    const std::size_t bucketSize =
        points.size() / numBuckets;

    table.reserve(numBuckets);

    for (std::size_t b = 0; b < numBuckets; ++b)
    {
        const std::size_t start = b * bucketSize;

        const std::size_t end =
            (b == numBuckets - 1)
                ? points.size()
                : start + bucketSize;

        if (start >= end)
        {
            continue;
        }

        std::size_t correct = 0;

        for (std::size_t i = start; i < end; ++i)
        {
            if (points[i].wasCorrect)
            {
                ++correct;
            }
        }

        CalibrationBucket bucket{};

        bucket.rawConfidenceLow =
            points[start].rawConfidence;

        bucket.rawConfidenceHigh =
            points[end - 1].rawConfidence;

        bucket.empiricalAccuracy =
            static_cast<double>(correct) /
            static_cast<double>(end - start);

        table.push_back(bucket);
    }

    /*
        Enforce monotonicity (simplified isotonic regression via
        pool-adjacent-violators): repeatedly merge any adjacent
        pair where accuracy decreases, until the whole sequence
        is non-decreasing. A single forward pass with averaging
        is sufficient for a small, pre-bucketed table like this.
    */
    bool changed = true;

    while (changed)
    {
        changed = false;

        for (std::size_t i = 1; i < table.size(); ++i)
        {
            if (table[i].empiricalAccuracy 
                table[i - 1].empiricalAccuracy)
            {
                const double merged =
                    (table[i].empiricalAccuracy +
                     table[i - 1].empiricalAccuracy) / 2.0;

                table[i].empiricalAccuracy = merged;
                table[i - 1].empiricalAccuracy = merged;

                changed = true;
            }
        }
    }

    return table;
}


/*
    Looks up the calibrated (empirically-measured) confidence
    for a given raw confidence score, using the bucket whose
    [low, high] range contains it.

    Falls back to returning rawConfidence unchanged if the table
    is empty (e.g. not enough training data was available to
    build one) — this keeps the system functional even before
    calibration data exists, rather than failing outright.

    Values outside the observed training range are clamped to
    the nearest edge bucket's accuracy, since we have no
    empirical evidence beyond what was observed in training.
*/
double lookupCalibratedConfidence(
    const std::vector<CalibrationBucket>& table,
    double rawConfidence
)
{
    if (table.empty())
    {
        return rawConfidence;
    }

    for (const auto& bucket : table)
    {
        if (rawConfidence >= bucket.rawConfidenceLow &&
            rawConfidence <= bucket.rawConfidenceHigh)
        {
            return bucket.empiricalAccuracy;
        }
    }

    if (rawConfidence < table.front().rawConfidenceLow)
    {
        return table.front().empiricalAccuracy;
    }

    return table.back().empiricalAccuracy;
}