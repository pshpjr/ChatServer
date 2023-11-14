#pragma once
#include <cstdlib>
#include <Windows.h>
#include "Profiler.h"

namespace _multiPool {
	namespace detail {
		const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;
	}
}

template <typename T,bool usePlacement = false>
class MultiThreadObjectPool
{
	struct Node
	{
		Node* head;
		T data;
		Node* next;
		Node() :next(nullptr) {}
		Node(const T& data) :next(nullptr), data(data) {}
	};

public:



	MultiThreadObjectPool(int baseAllocSize):_count(baseAllocSize)
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
		for (Node* p = _top; p != nullptr;)
		{
			p = (Node*)(_multiPool::detail::pointerMask & (long long)p);
			Node* next = p->next;
			if constexpr (!usePlacement)
			{
				p->~Node();
			}
			free(p);
			p = next;
		}
	}

	long Size() const { return _count; }

	T* Alloc()
	{
		Node* next;
		Node* retNode;

		for(;;)
		{
			Node* top = _top;

			if (top == nullptr)
			{
				break;
			}

			if (InterlockedCompareExchange64((__int64*)&_top, (__int64)((Node*)((unsigned long long)top & _multiPool::detail::pointerMask))->next, (__int64)top) == (__int64)top)
			{
				retNode = (Node*)((unsigned long long)top & _multiPool::detail::pointerMask);

				if constexpr (usePlacement)
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
		
		if(usePlacement)
		{
			data->~T();
		}
		Node* node = (Node*)((unsigned long long)data - offsetof(Node, data));

		Node* newTop = (Node*)((unsigned long long)(node) | ((unsigned long long)(InterlockedIncrement16(&topCount)) << 47));

		for (;;)
		{
			auto top = _top;
			node->next = top;
			if (InterlockedCompareExchange64((__int64*)&_top, (__int64)newTop, (__int64)top) == (__int64)top)
			{
				InterlockedIncrement(&_count);
				break;
			}
		}


	}

private:
	inline Node* createNode()
	{
		Node* node = (Node*)malloc(sizeof(Node));
		node->next = nullptr;
		if constexpr (!usePlacement)
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

