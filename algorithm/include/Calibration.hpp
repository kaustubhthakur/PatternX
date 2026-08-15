#pragma once

#include <cstddef>
#include <vector>

struct CalibrationPoint
{
    double rawConfidence;
    bool wasCorrect;
};

struct CalibrationBucket
{
    double rawConfidenceLow;
    double rawConfidenceHigh;
    double empiricalAccuracy;
};

std::vector<CalibrationBucket> buildCalibrationTable(
    std::vector<CalibrationPoint> points,
    std::size_t numBuckets = 10
);

double lookupCalibratedConfidence(
    const std::vector<CalibrationBucket>& table,
    double rawConfidence
);