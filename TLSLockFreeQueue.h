#pragma once
#include "Memorypool.h"
#include "TLSPool.h"
#include <thread>


template <typename T>
class TLSLockFreeQueue
{
	struct Node
	{
		Node* next = nullptr;
		T data;
		int queueID;
		Node() = default;
		Node(const T& data) : data(data), next(nullptr) {}
	};

public:
	TLSLockFreeQueue() : queueID(InterlockedIncrement(&ID))
	{
		Node* dummy = _pool.Alloc();

		dummy->next = (Node*)queueID;
		dummy->queueID = queueID;
		_head = dummy;
		_tail = dummy;
	}

	void Enqueue(const T& data)
	{
		Node* allocNode = _pool.Alloc();
		Node* newNode = allocNode;
		newNode->data = data;
		newNode->next = (Node*)queueID;
		newNode->queueID = queueID;

		while (true)
		{
			Node* tail = _tail;
			Node* tailNode = (Node*)((unsigned long long)tail & pointerMask);
			auto headCount = (unsigned short)((unsigned long long)tail >> 47);

			Node* newTail = (Node*)((unsigned long long)newNode | ((unsigned long long)(headCount + 1)) << 47);


			//auto debugCount = InterlockedIncrement64(&debugIndex);
			//auto index = debugCount % debugSize;

			//debug[index].threadID = std::this_thread::get_id();
			//debug[index].type = IoTypes::TryEnqueue;
			//debug[index].oldHead = (unsigned long long)tailNode;
			//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newNode & pointerMask));
			//debug[index].data = newNode->data;

			if (tailNode->next == (Node*)queueID)
			{
				if (InterlockedCompareExchangePointer((PVOID*)&tailNode->next, newTail, (Node*)queueID) == (Node*)queueID)
				{
	
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
		{
			return false;
		}

		bool find = false;

		while (true)
		{

			//내가 deq 하려고 했는데 남이 해버리면
			//나는 이 지점에 왔을 때 head next가 널이 되어버림.
			Node* head = _head;
			Node* headNode = (Node*)((unsigned long long)head & pointerMask);

			Node* next = headNode->next;
			Node* nextNode = (Node*)((unsigned long long)next & pointerMask);
			if (nextNode < (Node*)64000)
				return false;

			auto debugCount = InterlockedIncrement64(&debugIndex);
			//auto index = debugCount % debugSize;

			//debug[index].threadID = std::this_thread::get_id();
			//debug[index].type = IoTypes::TryDequeue;

			//debug[index].oldHead = (unsigned long long)headNode;
			//debug[index].newHead = (unsigned long long)nextNode;
			//debug[index].data = nextNode->data;


			auto headCount = (unsigned short)((unsigned long long)head >> 47);

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

	long Size() const { return _size; }

private:
	long _size = 0;
	Node* _head = nullptr;
	Node* _tail = nullptr;

	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;

	inline static TLSPool<Node, 0, false> _pool;


	static const int debugSize =1000;
	long long debugIndex = 0;
	//debugData<T> debug[debugSize];
	long long tailCount = 0;
	const int queueID;

	static long ID;

	long tryEnqueueCount = 0;

};
template <typename T>
long TLSLockFreeQueue<T>::ID = 0;