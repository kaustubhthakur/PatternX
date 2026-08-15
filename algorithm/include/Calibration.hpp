#pragma once

#include <cstddef>
#include <vector>

struct CalibrationPoint
{
    double rawConfidence = 0.0;
    bool wasCorrect = false;
};

struct CalibrationBucket
{
    double rawConfidenceLow = 0.0;
    double rawConfidenceHigh = 0.0;
    double empiricalAccuracy = 0.0;
};

std::vector<CalibrationBucket> buildCalibrationTable(
    std::vector<CalibrationPoint> points,
    std::size_t numBuckets = 10
);

double lookupCalibratedConfidence(
    const std::vector<CalibrationBucket>& table,
    double rawConfidence
);