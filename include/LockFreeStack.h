#pragma once
#include "MyWindows.h"
#include "MultiThreadObjectPool.h"
#include "LockFreeData.h"

template <typename T>
class LockFreeStack
{
    struct Node
    {
        T data{};
        Node* next = nullptr;
        Node() = default;

        Node(const T& data) : data(data)
                            , next(nullptr)
        {
        }
    };

public:
    LockFreeStack(): _pool(100)
    {
    }

    long size() const
    {
        return objectsInPool;
    }

    void Push(const T& data)
    {
        Node* node = _pool.Alloc();

        node->data = data;

        for (;;)
        {
            Node* top = _top;
            node->next = reinterpret_cast<Node*>((long long)top & lock_free_data::pointerMask);

            Node* newTop = reinterpret_cast<Node*>((unsigned long long)(node) | (
                                                  (long long)top + lock_free_data::indexInc &
                                                  lock_free_data::indexMask));

            if (InterlockedCompareExchange64(reinterpret_cast<__int64*>(&_top), reinterpret_cast<__int64>(newTop)
                                             , reinterpret_cast<__int64>(top)) == reinterpret_cast<__int64>(top))
            {
                InterlockedIncrement(&objectsInPool);

                break;
            }
        }
    }

    bool Pop(T& data)
    {
        for (;;)
        {
            Node* top = _top;

            Node* topNode = reinterpret_cast<Node*>((unsigned long long)top & lock_free_data::pointerMask);
            if (topNode == nullptr)
            {
                return false;
            }


            data = topNode->data;
            if (InterlockedCompareExchange64(reinterpret_cast<__int64*>(&_top)
                                             , reinterpret_cast<long long>(topNode->next) | reinterpret_cast<long long>(top) &
                                               lock_free_data::indexMask
                                             , reinterpret_cast<__int64>(top)) == reinterpret_cast<__int64>(top))
            {
                InterlockedDecrement(&objectsInPool);


                _pool.Free(topNode);


                break;
            }
        }
        return true;
    }

private:
    long objectsInPool = 0;

    Node* _top = nullptr;

    MultiThreadObjectPool<Node, false> _pool;
};
