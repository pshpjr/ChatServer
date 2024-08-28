#pragma once
#include <cstdlib>
#include "MyWindows.h"
#include "LockFreeData.h"

template <typename T, bool UsePlacement = false>
class MultiThreadObjectPool
{
    struct Node
    {
        Node* head;
        T data;
        Node* next;

        Node() : head(nullptr)
               , next(nullptr)
        {
        }

        explicit Node(const T& data) : head(nullptr)
                                     , data(data)
                                     , next(nullptr)
        {
        }
    };

public:
    MultiThreadObjectPool(const int baseAllocSize) : _count(baseAllocSize)
    {
        for (int i = 0; i < baseAllocSize; ++i)
        {
            auto newNode = createNode();
            newNode->next = _top;
            _top = newNode;
        }
    }

    ~MultiThreadObjectPool()
    {
        Node* p = reinterpret_cast<Node*>((long long)_top & lock_free_data::pointerMask);
        for (; p != nullptr;)
        {
            Node* next = p->next;
            if constexpr (!UsePlacement)
            {
                p->~Node();
            }
            free(p);
            p = next;
        }
    }

    long Size() const
    {
        return _count;
    }

    T* Alloc()
    {
        Node* retNode;

        for (;;)
        {
            Node* top = _top;

            Node* topNode = reinterpret_cast<Node*>((unsigned long long)top & lock_free_data::pointerMask);
            if (topNode == nullptr)
            {
                break;
            }

            if (InterlockedCompareExchange64(reinterpret_cast<__int64*>(&_top)
                                             , reinterpret_cast<long long>(topNode->next) | reinterpret_cast<long long>(top) &
                                               lock_free_data::indexMask
                                             , reinterpret_cast<__int64>(top)) == reinterpret_cast<__int64>(top))
            {
                retNode = reinterpret_cast<Node*>((unsigned long long)top & lock_free_data::pointerMask);

                if constexpr (UsePlacement)
                {
                    new(&retNode->data) T();
                }
                InterlockedDecrement(&_count);
                return &retNode->data;
            }
        }

        retNode = createNode();
        return &retNode->data;
    }

    void Free(T* data)
    {
        if (UsePlacement)
        {
            data->~T();
        }
        Node* node = reinterpret_cast<Node*>((unsigned long long)data - offsetof(Node, data));
        //Node* newTop = ( Node* ) ( ( unsigned long long )( node ) | ( ( unsigned long long )( InterlockedIncrement16(&topCount) ) << 47 ) );


        for (;;)
        {
            auto top = _top;
            node->next = reinterpret_cast<Node*>((long long)top & lock_free_data::pointerMask);
            Node* newTop = reinterpret_cast<Node*>((unsigned long long)(node) | (
                                                  ((long long)top + lock_free_data::indexInc) &
                                                  lock_free_data::indexMask));

            if (InterlockedCompareExchange64(reinterpret_cast<__int64*>(&_top), reinterpret_cast<__int64>(newTop)
                                             , reinterpret_cast<__int64>(top)) == reinterpret_cast<__int64>(top))
            {
                InterlockedIncrement(&_count);
                break;
            }
        }
    }

private:
    Node* createNode()
    {
        Node* node = reinterpret_cast<Node*>(malloc(sizeof(Node)));
        node->next = nullptr;
        if constexpr (!UsePlacement)
        {
            new(&node->data) T();
        }
        return node;
    }

private:
    Node* _top;

    short topCount = 0;
    long _count = 0;
};
