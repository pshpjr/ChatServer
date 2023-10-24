#pragma once
#include "Memorypool.h"
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
	void test(int expect)
	{
		int count = 0;
		Node* head = _head;
		Node* headNode = (Node*)((unsigned long long)head & pointerMask);
		head = headNode->next;
		while (head != nullptr) {
			//printf("%d\n", headNode->data);
			headNode = (Node*)((unsigned long long)head & pointerMask);
			head = headNode->next;
			count++;
		}
		printf("========= %d \n", count);
		if(count != expect)
			DebugBreak();
	}


	LockFreeQueue() : queueID(InterlockedIncrement(&ID))
	{
		Node* dummy = _pool.Alloc();
		
		dummy->next = nullptr;
		_head = dummy;
		_tail = dummy;
	}

	void Enqueue(const T& data)
	{
		Node* allocNode = _pool.Alloc();
		Node* newNode = allocNode;
		newNode->data = data;

		newNode->next = nullptr;
		

		while (isRun)
		{
			Node* tail = _tail;
			Node* tailNode = (Node*)((unsigned long long)tail & pointerMask);
			auto headCount = (unsigned short)((unsigned long long)tail >> 47);


			//auto debugCount = InterlockedIncrement64(&debugIndex);
			//auto index = debugCount % debugSize;



			//debug[index].threadID = std::this_thread::get_id();
			//debug[index].type = IoTypes::TryEnqueue;
			//debug[index].oldHead = (unsigned long long)tailNode;
			//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newNode & pointerMask));

			//debug[index].oldHead = (unsigned long long)tailNode->data;
			//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newTail &pointerMask))->data;

			if(InterlockedIncrement(&tryEnqueueCount)==10)
			{
				isRun = false;
				DebugBreak();
			}

			Node* newTail = (Node*)((unsigned long long)newNode | ((unsigned long long)(headCount + 1)) << 47);
			if (tailNode->next == NULL)
			{
				if (InterlockedCompareExchangePointer((PVOID*)&tailNode->next, newTail, nullptr) == nullptr)
				{
					InterlockedExchange(&tryEnqueueCount,0);
					//auto debugCount = InterlockedIncrement64(&debugIndex);
					//auto index = debugCount % debugSize;

					//debug[index].threadID = std::this_thread::get_id();
					//debug[index].type = IoTypes::Enqueue;
					//debug[index].oldHead = (unsigned long long)tailNode;
					//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newTail &pointerMask));



					if(InterlockedCompareExchangePointer((PVOID*)&_tail, newTail, tail)==tail)
					{

						//auto debugCount = InterlockedIncrement64(&debugIndex);
						//auto index = debugCount % debugSize;

						//debug[index].threadID = std::this_thread::get_id();
						//debug[index].type = IoTypes::changeTailEnqueue;

						//debug[index].oldHead = (unsigned long long)tailNode;
						//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newTail & pointerMask));


					}

					break;
				}
			}
			else
			{
				InterlockedCompareExchangePointer((PVOID*)&_tail, tailNode->next, tail);
				//auto debugCount = InterlockedIncrement64(&debugIndex);
				//auto index = debugCount % debugSize;

				//debug[index].threadID = std::this_thread::get_id();
				//debug[index].type = IoTypes::changeTailEnqueueFail;

				//debug[index].oldHead = (unsigned long long)tailNode;
				//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)tailNode->next & pointerMask));
			}
			
		}
		InterlockedIncrement(&_size);
	}

	bool Dequeue(T& data)
	{
		if (_size == 0)
			return false;
		while (isRun)
		{


			//내가 deq 하려고 했는데 남이 해버리면
			//나는 이 지점에 왔을 때 head next가 널이 되어버림.
			Node* head = _head;
			Node* headNode = (Node*)((unsigned long long)head & pointerMask);

			Node* next = headNode->next;
			Node* nextNode = (Node*)((unsigned long long)next & pointerMask);


			//auto debugCount = InterlockedIncrement64(&debugIndex);
			//auto index = debugCount % debugSize;

			//debug[index].threadID = std::this_thread::get_id();
			//debug[index].type = IoTypes::TryDequeue;

			//debug[index].oldHead = (unsigned long long)headNode;
			//debug[index].newHead = (unsigned long long)nextNode;

			if (next == nullptr)
			{
				continue;
			}

			auto headCount = (unsigned short)((unsigned long long)head >> 47);


			if (InterlockedCompareExchangePointer((PVOID*)&_head, next, head) == head)
			{
				//nextNode->data--;
				//if(nextNode->data!=0)
				//{
				//	DebugBreak();
				//}
				

	/*			auto debugCount = InterlockedIncrement64(&debugIndex);
				auto index = debugCount % debugSize;

				debug[index].threadID = std::this_thread::get_id();
				debug[index].type = IoTypes::Dequeue;

				debug[index].oldHead = (unsigned long long)headNode;
				debug[index].newHead = (unsigned long long)nextNode;*/


				Node* tail = _tail;
				Node* tailNode = (Node*)((unsigned long long)tail & pointerMask);
				if (tailNode->next != nullptr)
				{
					InterlockedCompareExchangePointer((PVOID*)&_tail, tailNode->next, tail);
					//debug[index].tail = (unsigned long long)tailNode->next;
				}

				data = nextNode->data;


		

				_pool.Free(headNode);
				break;
			}

		}
		InterlockedDecrement(&_size);

		return true;
	}

	long _size = 0;
private:
	Node* _head = nullptr;
	Node* _tail = nullptr;

	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;
	inline static MemoryPool<Node, 0, false> _pool { 10 };

	static const int debugSize = 3000;
	long long debugIndex = 0;
	debugData<T> debug[debugSize];
	long long tailCount = 0;
	const int queueID;

	static long ID;

	long tryEnqueueCount = 0;

};
template <typename T>
long LockFreeQueue<T>::ID = 0;