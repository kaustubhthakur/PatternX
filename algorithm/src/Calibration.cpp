#include "../include/Calibration.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>


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
        return table;
    }

  
    std::sort(
        points.begin(),
        points.end(),
        [](const CalibrationPoint& a,
           const CalibrationPoint& b)
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