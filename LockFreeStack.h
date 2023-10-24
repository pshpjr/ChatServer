#pragma once

#include <cstdio>
#include <Windows.h>
#include <thread>
#include "Memorypool.h"
#include "TLSPool.h"

template <typename T>
class LockFreeStack
{

	struct Node
	{
		Node* next = nullptr;
		T data=0;
		Node() = default;
		Node(const T& data) : data(data), next(nullptr) {}
	};


public:
	LockFreeStack():_pool(100)
	{
		
	}


	void Push(const T& data)
	{
		Node* node = _pool.Alloc();

		node->data = data;

		while (true)
		{
			Node* t = top;

			node->next = t;

			unsigned short topCount = (unsigned short)t + 1;

			Node* newTop = (Node*)((unsigned long long)(node) | ((unsigned long long)topCount << 47));

			if(InterlockedCompareExchange64((__int64*)&top, (__int64)newTop, (__int64)t) == (__int64)t)
			{


				break;	
			}
		}
	}

	void Pop(T& data)
	{
		while (true)
		{
			Node* t = top;

			Node* topNode = (Node*)((unsigned long long)t & pointerMask);

			Node* node = topNode->next;


			if (InterlockedCompareExchange64((__int64*)&top, (__int64)node, (__int64)t) == (__int64)t)
			{
				data = topNode->data;

				_pool.Free(topNode);


				break;
			}
		}
	}


private:
	Node* top=nullptr;

	short topCount = 0;
	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;
	MemoryPool<Node, 0, false> _pool;
};

template <typename T>
class TLSLockFreeStack
{

	struct Node
	{
		Node* next = nullptr;
		T data = 0;
		Node() = default;
		Node(const T& data) : data(data), next(nullptr) {}
	};


public:
	TLSLockFreeStack()
	{

	}


	void Push(const T& data)
	{
		Node* node = _pool.Alloc();

		node->data = data;

		while (true)
		{
			Node* t = top;

			node->next = t;

			unsigned short topCount = (unsigned short)t + 1;

			Node* newTop = (Node*)((unsigned long long)(node) | ((unsigned long long)topCount << 47));

			if (InterlockedCompareExchange64((__int64*)&top, (__int64)newTop, (__int64)t) == (__int64)t)
			{


				break;
			}
		}
	}

	void Pop(T& data)
	{
		while (true)
		{
			Node* t = top;

			Node* topNode = (Node*)((unsigned long long)t & pointerMask);

			Node* node = topNode->next;


			if (InterlockedCompareExchange64((__int64*)&top, (__int64)node, (__int64)t) == (__int64)t)
			{
				data = topNode->data;

				_pool.Free(topNode);


				break;
			}
		}
	}


private:
	Node* top = nullptr;

	short topCount = 0;
	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;
	static TLSPool<Node, 0, false> _pool;
};
