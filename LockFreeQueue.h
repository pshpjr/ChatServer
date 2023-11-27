#pragma once
#include "Memorypool.h"
#include "MultiThreadObjectPool.h"
#include <thread>



template <typename T>
class LockFreeQueue
{
	struct Node
	{

		Node* next = nullptr;
		T data;
		Node() = default;
		Node(const T& data) : data(data), next(nullptr) {}
	};

public:

	LockFreeQueue(int basePoolSize) : _pool(basePoolSize)
	{
		Node* dummy = _pool.Alloc();
		
		dummy->next = nullptr;
		_head = dummy;
		_tail = dummy;
	}

	long Size() const { return _size; }

	void Enqueue(const T& data)
	{
		Node* newNode = _pool.Alloc();
		newNode->data = data;

		newNode->next = nullptr;
		auto nextTailCount = InterlockedIncrement16(&tailCount);
		Node* newTail = (Node*)((unsigned long long)newNode | ((unsigned long long)(nextTailCount)) << 47);



		while (true)
		{
			Node* tail = _tail;
			Node* tailNode = (Node*)((unsigned long long)tail & pointerMask);
			auto enqCount = InterlockedIncrement(&tryEnqueueCount);
			if (enqCount == 100)
			{
				DebugBreak();
			}


			if (tailNode->next == nullptr)
			{
				if (InterlockedCompareExchangePointer((PVOID*)&tailNode->next, newTail, nullptr) == nullptr)
				{
					InterlockedExchange(&tryEnqueueCount, 0);
					//auto debugCount = InterlockedIncrement64(&debugIndex);
					//auto index = debugCount % debugSize;

					//debug[index].threadID = std::this_thread::get_id();
					//debug[index].type = IoTypes::Enqueue;
					//debug[index].oldHead = (unsigned long long)tailNode;
					//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newTail &pointerMask));

					if(InterlockedCompareExchangePointer((PVOID*)&_tail, newTail, tail)==tail)
					{

					}

					break;
				}
			}
			else
			{
				InterlockedCompareExchangePointer((PVOID*)&_tail, tailNode->next, tail);
			}
			
		}
		InterlockedIncrement(&_size);
	}

	bool Dequeue(T& data)
	{
		if (_size == 0)
			return false;

		while (true)
		{
			Node* head = _head;
			Node* headNode = (Node*)((unsigned long long)head & pointerMask);

			Node* next = headNode->next;
			Node* nextNode = (Node*)((unsigned long long)next & pointerMask);

			if (next == nullptr)
				return false;

			data = nextNode->data;

			if (InterlockedCompareExchangePointer((PVOID*)&_head, next, head) == head)
			{

				Node* tail = _tail;
				Node* tailNode = (Node*)((unsigned long long)tail & pointerMask);
				if (tail == head)
				{
					//내가 뺀 애가 tail이면 풀에 넣기 전에 수정해줘야 함. 아니면 꼬임.
					InterlockedCompareExchangePointer((PVOID*)&_tail, tailNode->next, tail);
				}

				_pool.Free(headNode);
				break;
			}

		}
		InterlockedDecrement(&_size);

		return true;
	}


private:
	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;

	Node* _head = nullptr;
	Node* _tail = nullptr;
	short tailCount = 0;
	static long ID;
	long _size = 0;
	MultiThreadObjectPool<Node, false> _pool;


	//DEBUG
	static const int debugSize = 3000;
	long long debugIndex = 0;
	//debugData<T> debug[debugSize];


	long tryEnqueueCount = 0;

};
template <typename T>
long LockFreeQueue<T>::ID = 0;