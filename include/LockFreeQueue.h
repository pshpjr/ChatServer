#pragma once
#include "LockFreeData.h"
#include "MultiThreadObjectPool.h"


template <typename T>
class LockFreeQueue
{
    union Pointer
    {
        struct
        {
            short tmp1;
            short tmp2;
            short tmp3;
            short index;
        };

        long pointer;
    };


    struct Node
    {
        Node* next = nullptr;
        T data;
        Node() = default;

        explicit Node(const T& data)
            : next(nullptr)
            , data(data)
        {
        }
    };

public:
    explicit LockFreeQueue(int basePoolSize)
        : _pool(basePoolSize)
    {
        Node* dummy = _pool.Alloc();

        dummy->next = nullptr;
        _head = dummy;
        _tail = dummy;
    }

    long Size() const
    {
        return _size;
    }

    int Enqueue(const T& data)
    {
        Node* newNode = _pool.Alloc();
        newNode->data = data;

        newNode->next = nullptr;
        const auto nextTailCount = InterlockedIncrement16(&tailCount);
        Node* newTail = static_cast<Node*>(static_cast<unsigned long long>(newNode) | static_cast<unsigned long long>(
            nextTailCount) << 47);


        int loop = 0;
        while (true)
        {
            Node* tail = _tail;
            Node* tailNode = static_cast<Node*>(static_cast<unsigned long long>(tail) & lock_free_data::pointerMask);

            if (tailNode->next == nullptr)
            {
                if (InterlockedCompareExchangePointer(static_cast<PVOID*>(&tailNode->next), newTail, nullptr) ==
                    nullptr)
                {
                    //auto debugCount = InterlockedIncrement64(&debugIndex);
                    //auto index = debugCount % debugSize;

                    //debug[index].threadID = std::this_thread::get_id();
                    //debug[index].type = IoTypes::Enqueue;
                    //debug[index].oldHead = (unsigned long long)tailNode;
                    //debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newTail &lock_free_data::_tailCount));

                    InterlockedCompareExchangePointer(static_cast<PVOID*>(&_tail), newTail, tail);

                    break;
                }
            }
            else
            {
                InterlockedCompareExchangePointer(static_cast<PVOID*>(&_tail), tailNode->next, tail);
            }
            loop++;
        }
        InterlockedIncrement(&_size);
        return loop;
    }

    int Dequeue(T& data)
    {
        if (_size == 0)
        {
            return -1;
        }

        int loopCount = 0;
        while (true)
        {
            Node* head = _head;
            Node* headNode = static_cast<Node*>(static_cast<unsigned long long>(head) & lock_free_data::pointerMask);

            Node* next = headNode->next;
            Node* nextNode = static_cast<Node*>(static_cast<unsigned long long>(next) & lock_free_data::pointerMask);

            if (next == nullptr)
            {
                return false;
            }

            data = nextNode->data;

            if (InterlockedCompareExchangePointer(static_cast<PVOID*>(&_head), next, head) == head)
            {
                Node* tail = _tail;
                Node* tailNode = static_cast<Node*>(static_cast<unsigned long long>(tail) & lock_free_data::pointerMask);
                if (tail == head)
                {
                    //내가 뺀 애가 tail이면 풀에 넣기 전에 수정해줘야 함. 아니면 꼬임.
                    InterlockedCompareExchangePointer(static_cast<PVOID*>(&_tail), tailNode->next, tail);
                }

                _pool.Free(headNode);
                break;
            }
            loopCount++;
        }
        InterlockedDecrement(&_size);

        return loopCount;
    }

private:
    //이거 붙어있을 때 떨어져 있을 때 성능 차이 비교
    Node* _head = nullptr;
    Node* _tail = nullptr;
    //얘도 계속 수정됨. 따로 있을 때 비교.
    short tailCount = 0;
    static long ID;
    long _size = 0;
    MultiThreadObjectPool<Node> _pool;


    //DEBUG
    static constexpr int debugSize = 3000;
    long long debugIndex = 0;
    //debugData<T> debug[debugSize];
};

template <typename T>
long LockFreeQueue<T>::ID = 0;
