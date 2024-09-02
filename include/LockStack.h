#pragma once
#include "MyWindows.h"

template <typename T>
class LockStack
{
    struct Node
    {
        Node* next = nullptr;
        T data = 0;
        Node() = default;

        Node(const T& data)
            : next(nullptr)
            , data(data)
        {
        }
    };

public:
    LockStack()
    {
        InitializeCriticalSection(&_cs);
    }

    void Push(const T& data)
    {
        Node* node = new Node();

        node->data = data;

        EnterCriticalSection(&_cs);
        node->next = _top;
        _top = node;
        LeaveCriticalSection(&_cs);
    }

    T Pop()
    {
        EnterCriticalSection(&_cs);
        Node* t = _top;
        _top = t->next;
        LeaveCriticalSection(&_cs);

        T ret = t->data;
        delete t;
        return ret;
    }

private:
    Node* _top = nullptr;
    int _pooledNodeCount = 0;
    CRITICAL_SECTION _cs;
};
