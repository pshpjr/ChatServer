#pragma once
#include "Container.h"

template <typename T, int BufferSize>
class LockFreeFixedQueue
{
    static_assert((BufferSize & BufferSize - 1) == 0);

    struct Node
    {
        char isUsed = 0;
        T data = T();
    };

public:
    constexpr LockFreeFixedQueue() : indexMask(BufferSize - 1)
                                   , buffer(BufferSize)
    {
    }

    LockFreeFixedQueue(const LockFreeFixedQueue& other) = delete;
    LockFreeFixedQueue(LockFreeFixedQueue&& other) = delete;

    LockFreeFixedQueue& operator =(const LockFreeFixedQueue& other) = delete;
    LockFreeFixedQueue& operator =(LockFreeFixedQueue&& other) = delete;

    ~LockFreeFixedQueue() = default;

    int Size() const
    {
        auto head = headIndex & indexMask;
        auto tail = tailIndex & indexMask;

        if (tail >= head)
        {
            return tail - head;
        }

        return BufferSize - (head - tail);
    }

    bool Enqueue(const T& data)
    {
        const long tail = InterlockedIncrement(&tailIndex) - 1;

        //while () {};

        if (buffer[tail & indexMask].isUsed == 1)
        {
            return false;
        }

        buffer[tail & indexMask].data = data;
        if (InterlockedExchange8(&buffer[tail & indexMask].isUsed, 1) == 1)
        {
            __debugbreak();
        }
        return true;
    }

    bool Dequeue(T& data)
    {
        long head;
        for (;;)
        {
            head = headIndex;

            if (buffer[head & indexMask].isUsed == false)
            {
                return false;
            }

            if (InterlockedCompareExchange(&headIndex, head + 1, head) == head)
            {
                break;
            }
        }
        data = std::move(buffer[head & indexMask].data);
        
        if constexpr (std::is_class_v<T> && !std::is_move_assignable_v<T>)
        {
            buffer[head & indexMask].~T();
        }
        
        if (InterlockedExchange8(&buffer[head & indexMask].isUsed, false) == false)
        {
            __debugbreak();
        }
        return true;
    }

#pragma warning (disable : 4324)
    alignas( 64 ) long headIndex = 0;
    alignas( 64 ) long tailIndex = 0;
#pragma warning (default : 4324)
    int indexMask;
    Vector<Node> buffer;

public:
};
