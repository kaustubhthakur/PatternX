#include "../include/Calibration.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

/*
    Builds an empirical calibration table from training-period
    (rawConfidence, wasCorrect) observations.

    Points are sorted by raw confidence and split into
    approximately equal-sized buckets.

    Each bucket stores:
        - raw confidence range
        - empirical accuracy
        - number of observations

    A proper weighted Pool Adjacent Violators Algorithm (PAVA)
    is then applied to guarantee that calibrated confidence is
    monotonically non-decreasing with raw confidence.

    Returns an empty table if there isn't enough data to form
    reliable buckets.
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

    /*
        Require at least 3 observations per bucket.
    */
    if (points.size() < numBuckets * 3)
    {
        return table;
    }

    /*
        Sort observations by raw confidence.
    */
    std::sort(
        points.begin(),
        points.end(),
        [](const CalibrationPoint& a,
           const CalibrationPoint& b)
        {
            return a.rawConfidence < b.rawConfidence;
        }
    );

    /*
        Equal-sized buckets.

        The final bucket receives any remaining observations.
    */
    const std::size_t bucketSize =
        points.size() / numBuckets;

    table.reserve(numBuckets);

    for (std::size_t b = 0; b < numBuckets; ++b)
    {
        const std::size_t start =
            b * bucketSize;

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

    if (table.size() <= 1)
    {
        return table;
    }

    /*
        Proper weighted Pool Adjacent Violators Algorithm.

        Since CalibrationBucket currently does not store its
        observation count, reconstruct the bucket weight from
        the original equal-sized buckets.

        The last bucket may contain more observations.
    */
    struct PavaBlock
    {
        std::size_t first;
        std::size_t last;

        double weightedAccuracy;
        std::size_t weight;
    };

    std::vector<PavaBlock> blocks;
    blocks.reserve(table.size());

    for (std::size_t i = 0; i < table.size(); ++i)
    {
        /*
            Reconstruct bucket weight.

            Normal buckets contain bucketSize observations.
            The final bucket contains all remaining observations.
        */
        std::size_t weight = bucketSize;

        if (i == table.size() - 1)
        {
            weight =
                points.size() -
                (table.size() - 1) * bucketSize;
        }

        PavaBlock block{};

        block.first = i;
        block.last = i;
        block.weight = weight;
        block.weightedAccuracy =
            table[i].empiricalAccuracy;

        blocks.push_back(block);

        /*
            Merge violating adjacent blocks.

            Earlier confidence bucket must not have
            greater calibrated accuracy than a later bucket.
        */
        while (blocks.size() >= 2)
        {
            const std::size_t right =
                blocks.size() - 1;

            const std::size_t left =
                right - 1;

            if (blocks[left].weightedAccuracy <=
                blocks[right].weightedAccuracy)
            {
                break;
            }

            PavaBlock merged{};

            merged.first =
                blocks[left].first;

            merged.last =
                blocks[right].last;

            merged.weight =
                blocks[left].weight +
                blocks[right].weight;

            merged.weightedAccuracy =
                (
                    blocks[left].weightedAccuracy *
                    static_cast<double>(blocks[left].weight)
                    +
                    blocks[right].weightedAccuracy *
                    static_cast<double>(blocks[right].weight)
                )
                /
                static_cast<double>(merged.weight);

            blocks.pop_back();
            blocks.pop_back();

            blocks.push_back(merged);
        }
    }

    /*
        Apply the calibrated values back to the buckets.
    */
    for (const auto& block : blocks)
    {
        for (std::size_t i = block.first;
             i <= block.last;
             ++i)
        {
            table[i].empiricalAccuracy =
                block.weightedAccuracy;
        }
    }

    return table;
}


/*
    Looks up calibrated confidence for a raw confidence value.

    If rawConfidence falls inside a bucket's observed range,
    that bucket's empirical accuracy is returned.

    Values below/above the observed range are clamped to the
    nearest bucket's calibrated accuracy.

    If the calibration table is empty, raw confidence is
    returned unchanged.
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

    /*
        Find the bucket containing the confidence value.
    */
    for (const auto& bucket : table)
    {
        if (rawConfidence >= bucket.rawConfidenceLow &&
            rawConfidence <= bucket.rawConfidenceHigh)
        {
            return bucket.empiricalAccuracy;
        }
    }

    /*
        Below observed training range.
    */
    if (rawConfidence < table.front().rawConfidenceLow)
    {
        return table.front().empiricalAccuracy;
    }

    /*
        Above observed training range.
    */
    return table.back().empiricalAccuracy;
}