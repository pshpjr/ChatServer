#pragma once
#include "Memorypool.h"
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

	int Enqueue(const T& data)
	{
		Node* newNode = _pool.Alloc();
		newNode->data = data;

		newNode->next = nullptr;
		auto nextTailCount = InterlockedIncrement16(&tailCount);
		Node* newTail = (Node*)((unsigned long long)newNode | ((unsigned long long)(nextTailCount)) << 47);


		int loop = 0;
		while (true)
		{

			Node* tail = _tail;
			Node* tailNode = (Node*)((unsigned long long)tail & pointerMask);

			if (tailNode->next == nullptr)
			{
				if (InterlockedCompareExchangePointer((PVOID*)&tailNode->next, newTail, nullptr) == nullptr)
				{
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
			loop++;
		}
		InterlockedIncrement(&_size);
		return loop;
	}

	int Dequeue(T& data)
	{
		if (_size == 0)
			return -1;

		int loopCount = 0;
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
			loopCount++;
		}
		InterlockedDecrement(&_size);

		return loopCount;
	}


private:
	const unsigned long long pointerMask = 0x0000'7FFF'FFFF'FFFF;

	//이거 붙어있을 때 떨어져 있을 때 성능 차이 비교
	Node* _head = nullptr;
	Node* _tail = nullptr;
	//얘도 계속 수정됨. 따로 있을 때 비교.
	short tailCount = 0;
	static long ID;
	long _size = 0;
	MultiThreadObjectPool<Node, false> _pool;


	//DEBUG
	static const int debugSize = 3000;
	long long debugIndex = 0;
	//debugData<T> debug[debugSize];



};
template <typename T>
long LockFreeQueue<T>::ID = 0;