#pragma once
#include <cstdlib>
#include <Windows.h>

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
		Node* retNode = nullptr;
		while (true)
		{
			Node* top = _top;

			if (top == nullptr)
			{
				break;
			}

			Node* topNode = (Node*)((unsigned long long)top & pointerMask);
			Node* next = topNode->next;

			if (InterlockedCompareExchange64((__int64*)&_top, (__int64)next, (__int64)top) == (__int64)top)
			{
				retNode = topNode;
				break;
			}
		}

		if(retNode == nullptr)
		{
			retNode = createNode();
		}
		else
		{
			InterlockedDecrement(&_count);
		}

		if constexpr (usePlacement)
		{
			new(&retNode->data) T();
		}

		return &retNode->data;
	}

	void Free(T* data)
	{
		if(usePlacement)
		{
			data->~T();
		}

		auto _topCount = InterlockedIncrement16(&topCount);

		Node* node = (Node*)((unsigned long long)data - offsetof(Node, data));
		Node* newTop = (Node*)((unsigned long long)(node) | ((unsigned long long)_topCount << 47));

		while (true)
		{
			Node* top = _top;
			node->next = top;
			if (InterlockedCompareExchange64((__int64*)&_top, (__int64)newTop, (__int64)top) == (__int64)top)
			{
				InterlockedIncrement(&_count);
				break;
			}
		}
	}

private:
	Node* createNode()
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
	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;

	long _count;
};

