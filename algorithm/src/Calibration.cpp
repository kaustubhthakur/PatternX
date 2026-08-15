#include "../include/Calibration.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace
{

struct PavaBlock
{
    std::size_t first = 0;
    std::size_t last = 0;
    double accuracy = 0.0;
    std::size_t weight = 0;
};

}

std::vector<CalibrationBucket> buildCalibrationTable(
    std::vector<CalibrationPoint> points,
    std::size_t numBuckets
)
{
    std::vector<CalibrationBucket> table;

    if (numBuckets == 0 || points.size() < numBuckets * 3)
    {
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

        CalibrationBucket bucket;

        bucket.rawConfidenceLow =
            points[start].rawConfidence;

        bucket.rawConfidenceHigh =
            points[end - 1].rawConfidence;

        bucket.empiricalAccuracy =
            static_cast<double>(correct) /
            static_cast<double>(end - start);

        table.push_back(bucket);
    }

    if (table.empty())
    {
        return table;
    }

    std::vector<PavaBlock> blocks;
    blocks.reserve(table.size());

    for (std::size_t i = 0; i < table.size(); ++i)
    {
        const std::size_t weight =
            (i == table.size() - 1)
                ? points.size() -
                    (table.size() - 1) * bucketSize
                : bucketSize;

        PavaBlock block;

        block.first = i;
        block.last = i;
        block.accuracy = table[i].empiricalAccuracy;
        block.weight = weight;

        blocks.push_back(block);

        while (blocks.size() >= 2)
        {
            const std::size_t right =
                blocks.size() - 1;

            const std::size_t left =
                right - 1;

            if (blocks[left].accuracy <=
                blocks[right].accuracy)
            {
                break;
            }

            PavaBlock merged;

            merged.first =
                blocks[left].first;

            merged.last =
                blocks[right].last;

            merged.weight =
                blocks[left].weight +
                blocks[right].weight;

            merged.accuracy =
                (
                    blocks[left].accuracy *
                    static_cast<double>(blocks[left].weight)
                    +
                    blocks[right].accuracy *
                    static_cast<double>(blocks[right].weight)
                )
                /
                static_cast<double>(merged.weight);

            blocks.pop_back();
            blocks.pop_back();

            blocks.push_back(merged);
        }
    }

    for (const auto& block : blocks)
    {
        for (std::size_t i = block.first;
             i <= block.last;
             ++i)
        {
            table[i].empiricalAccuracy =
                block.accuracy;
        }
    }

    return table;
}

double lookupCalibratedConfidence(
    const std::vector<CalibrationBucket>& table,
    double rawConfidence
)
{
    if (table.empty())
    {
        return rawConfidence;
    }

    rawConfidence =
        std::max(0.0, std::min(1.0, rawConfidence));

    if (rawConfidence <= table.front().rawConfidenceLow)
    {
        return table.front().empiricalAccuracy;
    }

    if (rawConfidence >= table.back().rawConfidenceHigh)
    {
        return table.back().empiricalAccuracy;
    }

    for (std::size_t i = 0; i + 1 < table.size(); ++i)
    {
        const CalibrationBucket& left = table[i];
        const CalibrationBucket& right = table[i + 1];

        if (rawConfidence > left.rawConfidenceHigh &&
            rawConfidence < right.rawConfidenceLow)
        {
            const double x1 = left.rawConfidenceHigh;
            const double x2 = right.rawConfidenceLow;

            if (x2 <= x1)
            {
                return right.empiricalAccuracy;
            }

            const double t =
                (rawConfidence - x1) /
                (x2 - x1);

            return left.empiricalAccuracy +
                   t *
                   (right.empiricalAccuracy -
                    left.empiricalAccuracy);
        }

        if (rawConfidence >= left.rawConfidenceLow &&
            rawConfidence <= left.rawConfidenceHigh)
        {
            return left.empiricalAccuracy;
        }
    }

    return table.back().empiricalAccuracy;
}