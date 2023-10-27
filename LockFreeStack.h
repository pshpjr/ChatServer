#pragma once

#include <cstdio>
#include <Windows.h>
#include <thread>
#include "MultiThreadObjectPool.h"
#include "TLSPool.h"

template <typename T>
class LockFreeStack
{
	struct Node
	{
		Node* next = nullptr;
		T data;
		Node() = default;
		Node(const T& data) : data(data), next(nullptr) {}
	};

public:
	LockFreeStack():_pool(100)
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

		auto _topCount = InterlockedIncrement16(&topCount);
		Node* newTop = (Node*)((unsigned long long)(node) | ((unsigned long long)_topCount << 47));

		while (true)
		{
			Node* t = top;

			node->next = t;

			if(InterlockedCompareExchange64((__int64*)&top, (__int64)newTop, (__int64)t) == (__int64)t)
			{
				InterlockedIncrement(&objectsInPool);

				break;	
			}
		}
	}

	bool Pop(T& data)
	{
		while (true)
		{
			Node* t = top;

			Node* topNode = (Node*)((unsigned long long)t & pointerMask);
			if(t == nullptr)
			{
				return false;
			}


			Node* node = topNode->next;

			data = topNode->data;
			if (InterlockedCompareExchange64((__int64*)&top, (__int64)node, (__int64)t) == (__int64)t)
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

	Node* top=nullptr;

	short topCount = 0;
	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;
	MultiThreadObjectPool<Node, false> _pool;
};
