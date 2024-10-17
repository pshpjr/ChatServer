﻿#pragma once
#include "MyWindows.h"
#include "Container.h"
#include "Macro.h"


template <typename T, int BufferSize>
class LockFreeFixedQueue
{
    static_assert((BufferSize & (BufferSize - 1)) == 0);

    struct Node
    {
        std::atomic<bool> isUsed = false;
        T data = T();
    };

public:
    constexpr LockFreeFixedQueue()
        : indexMask(BufferSize - 1)
        , buffer(BufferSize) {}

    LockFreeFixedQueue(const LockFreeFixedQueue& other) = delete;
    LockFreeFixedQueue(LockFreeFixedQueue&& other) = delete;

    LockFreeFixedQueue& operator =(const LockFreeFixedQueue& other) = delete;
    LockFreeFixedQueue& operator =(LockFreeFixedQueue&& other) = delete;

    ~LockFreeFixedQueue() = default;

    int Size() const
    {
        auto head = headIndex.load();
        auto tail = tailIndex.load();

        return tail - head;
    }

    bool Enqueue(const T& data)
    {
        const int tail = tailIndex.fetch_add(1);

        if (buffer[tail & indexMask].isUsed.load() == true)
        {
            return false;
        }

        buffer[tail & indexMask].data = std::move(data);
        if (buffer[tail & indexMask].isUsed.exchange(true) == true)
        {
            __debugbreak();
        }
        return true;
    }

    bool Dequeue(T& data)
    {
        int head;
        for (;;)
        {
            head = headIndex.load();

            if (buffer[head & indexMask].isUsed.load() == false)
            {
                return false;
            }

            if (headIndex.compare_exchange_strong(head, head + 1))
            {
                break;
            }
        }
        data = std::move(buffer[head & indexMask].data);


        if (buffer[head & indexMask].isUsed.exchange(false) == false)
        {
            __debugbreak();
        }

        return true;
    }

#pragma warning (disable : 4324)
    alignas( 64 ) std::atomic<int> headIndex = 0;
    alignas( 64 ) std::atomic<int> tailIndex = 0;
#pragma warning (default : 4324)
    const int indexMask;
    Vector<Node> buffer;

public:
};
