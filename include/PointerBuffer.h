﻿#pragma once
#include <type_traits>
#include "Container.h"
template <typename T, int N>

class PointerBuffer
{
    static_assert(std::is_pointer_v<T>, "T must be a pointer type");
    static_assert((N & N - 1) == 0, "N must be a power of 2");

    Array<T, N> _buffer;
    std::size_t _frontIndex = 0;
    std::size_t _rearIndex = 0;
    static const int INDEX_MASK = N - 1;

public:
    void Enqueue(T value)
    {
        if (_rearIndex - _frontIndex == N)
        {
            __debugbreak();
        }
        _buffer[_rearIndex & INDEX_MASK] = value;
        ++_rearIndex;
    }

    T Dequeue()
    {
        if (_frontIndex == _rearIndex)
        {
            return nullptr;
        }
        T value = _buffer[_frontIndex & INDEX_MASK];
        ++_frontIndex;
        return value;
    }

    void BatchDequeue(void (*fn)(T))
    {
        const size_t deqSize = size();
        for (int i = 0; i < deqSize; ++i)
        {
            fn(_buffer[_frontIndex & INDEX_MASK]);
            ++_frontIndex;
        }
    }

    [[nodiscard]] unsigned long long size() const noexcept
    {
        if (_rearIndex >= _frontIndex)
        {
            return (_rearIndex & INDEX_MASK) - (_frontIndex & INDEX_MASK);
        }

        return (_frontIndex & INDEX_MASK) - (_rearIndex & INDEX_MASK);
    }
};
