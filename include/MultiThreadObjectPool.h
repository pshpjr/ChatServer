#pragma once
#include <cstdlib>
#include "MyWindows.h"
#include "LockFreeData.h"

template <typename T, bool UsePlacement = false>
class MultiThreadObjectPool
{
    using storageType = std::aligned_storage_t<sizeof(T),alignof(T)>;
    struct Node
    {
        Node* head{nullptr};
        storageType data;
        Node* next{nullptr};

        explicit Node(const T& data) : data(data)
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

    template<typename ...Args>
    T* Alloc(Args... args)
    {
        Node* retNode;

        for (;;)
        {
            Node* top = _top;

            Node* topNode = reinterpret_cast<Node*>((unsigned long long)top & lock_free_data::pointerMask);
            if (topNode == nullptr)
            {
                retNode = createNode();
                if constexpr (!UsePlacement)
                {
                    std::construct_at(reinterpret_cast<T*>(&retNode->data),std::forward<Args>(args)...);
                }
                break;
            }

            if (InterlockedCompareExchange64(reinterpret_cast<__int64*>(&_top)
                                             , reinterpret_cast<long long>(topNode->next) | reinterpret_cast<long long>(top) &
                                               lock_free_data::indexMask
                                             , reinterpret_cast<__int64>(top)) == reinterpret_cast<__int64>(top))
            {
                retNode = reinterpret_cast<Node*>((unsigned long long)top & lock_free_data::pointerMask);

                InterlockedDecrement(&_count);
            }
        }

        if constexpr (UsePlacement)
        {
            std::construct_at(reinterpret_cast<T*>(&retNode->data),std::forward<Args>(args)...);
        }

        return &retNode->data;
    }

    void Free(T* data)
    {
        if (UsePlacement)
        {
            std::destroy_at(data);
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
        Node* node = new Node();
        return node;
    }

private:
    Node* _top;

    short topCount = 0;
    long _count = 0;
};
