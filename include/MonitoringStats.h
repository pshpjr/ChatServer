﻿#pragma once
#include <algorithm>

class MonitoringStats
{
public:
    void AddData(const int data)
    {
        _count++;
        _sum += data;
        _max = std::max(data, _max);
        _min = std::min(data, _min);
    }

    void Clear()
    {
        _max = 0;
        _min = INT_MAX;
        _average = 0;
        _count = 0;
        _sum = 0;
    }

    [[nodiscard]] double Avg() const
    {
        if (_count == 0)
        {
            return 0;
        }

        auto result = _sum / static_cast<double>(_count);
        if (isnan(result))
        {
            return 0;
        }

        return result;
    }

    [[nodiscard]] int Max() const
    {
        return _max;
    }

    [[nodiscard]] int Min() const
    {
        if (_count == 0)
        {
            return 0;
        }

        return _min;
    }

    [[nodiscard]] int Count() const
    {
        return _count;
    }

    [[nodiscard]] long long Sum() const
    {
        return _sum;
    }

private:
    int _max = 0;
    int _min = INT_MAX;
    double _average = 0;
    int _count = 0;
    long long _sum = 0;
};
