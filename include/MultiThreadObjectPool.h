#pragma once
#include <cstdlib>
#include <concepts>
#include <memory>
#include "LockFreeData.h"

template <typename T, bool UsePlacement = false>
requires (UsePlacement || std::default_initializable<T>)
class MultiThreadObjectPool
{
    using storageType = std::aligned_storage_t<sizeof(T),alignof(T)>;
    struct Node
    {
        Node* head{nullptr};
        storageType data{};
        Node* next{nullptr};

        Node() = default;

        explicit Node(const T& data) : head(nullptr)
                                     , data(data)
                                     , next(nullptr)
        {
        }
    };

public:
    template <typename ...Args>
    requires (UsePlacement || sizeof...(Args) == 0)
    explicit MultiThreadObjectPool(const int baseAllocSize,Args... args) : _count(baseAllocSize)
    {
        for (int i = 0; i < baseAllocSize; ++i)
        {
            auto newNode = createNode(std::forward<Args>(args)...);
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

    template <typename ...Args>
    requires (UsePlacement || sizeof...(Args) == 0)
    T* Alloc(Args... args)
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
                    std::construct_at(reinterpret_cast<T*>(&retNode->data),std::forward<Args>(args)...);
                }
                InterlockedDecrement(&_count);
                return reinterpret_cast<T*>(&retNode->data);
            }
        }

        retNode = createNode(std::forward<Args>(args)...);
        return reinterpret_cast<T*>(&retNode->data);
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
    template <typename ...Args>
    requires (UsePlacement || sizeof...(Args) == 0)
    Node* createNode(Args... args)
    {
        Node* node = new Node();
        if constexpr (!UsePlacement)
        {
            std::construct_at(node,std::forward<Args>(args)...);
        }
        return node;
    }


private:
    Node* _top{nullptr};

    short topCount = 0;
    long _count = 0;
};
