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
		T data {};
		Node() = default;
		Node(const T& data) : data(data), next(nullptr) {}
	};

public:
	TLSLockFreeQueue() : queueID((Node*)InterlockedIncrement64(&GID))
	{
		Node* dummy = _tlsLFQNodePool.Alloc();

		dummy->next = queueID;
		_head = dummy;
		_tail = dummy;
	}

	void Enqueue(const T& data)
	{
		Node* allocNode; {
			PRO_BEGIN("GET_NODE")
			allocNode = _tlsLFQNodePool.Alloc();
		}
		 
		Node* newNode = allocNode;
		newNode->data = data;
		newNode->next = queueID;

		auto headCount = InterlockedIncrement16(&tailCount);
		Node* newTail = ( Node* ) ( ( unsigned long long )newNode | ( ( unsigned long long )( headCount ) ) << 47 );
		{
			PRO_BEGIN("Enqueue_Loop")
			while ( true )
			{
				Node* tail = _tail;
				Node* tailNode = ( Node* ) ( ( unsigned long long )tail & pointerMask );

				if ( tailNode->next == queueID )
				{
					if ( InterlockedCompareExchangePointer(( PVOID* ) &tailNode->next, newTail, ( PVOID* ) queueID) == queueID )
					{

						if ( InterlockedCompareExchangePointer(( PVOID* ) &_tail, newTail, tail) == tail )
						{
						}

						break;
					}
				}
				//tail의 next가 queueiD가 아니면 경우의 수는 2가지
				//다른 큐의 id이거나 이미 tail 뒤에 누가 넣었거나, 다른 큐에 들어갔거나
				//내 큐의 tail이라면 cas 통과하지만, 그 외의 경우에는 _tail과 다를 것임. 
				else
				{
					InterlockedCompareExchangePointer(( PVOID* ) &_tail, tailNode->next, tail);
				}

			}
		}
		InterlockedIncrement(&_size);




		//auto debugCount = InterlockedIncrement64(&debugIndex);
		//auto index = debugCount % debugSize;

		//debug[index].threadID = std::this_thread::get_id();
		//debug[index].type = IoTypes::TryEnqueue;
		//debug[index].oldHead = (unsigned long long)tailNode;
		//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newNode & pointerMask));
		//debug[index].data = newNode->data;
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

			//auto debugCount = InterlockedIncrement64(&debugIndex);
			//auto index = debugCount % debugSize;

			//debug[index].threadID = std::this_thread::get_id();
			//debug[index].type = IoTypes::TryDequeue;

			//debug[index].oldHead = (unsigned long long)headNode;
			//debug[index].newHead = (unsigned long long)nextNode;
			//debug[index].data = nextNode->data;

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


				_tlsLFQNodePool.Free(headNode);
				break;
			}

		}
		InterlockedDecrement(&_size);

		return true;
	}

	long Size() const { return _size; }

private:
	Node* _head = nullptr;

	long _size = 0;



	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;

	int initSize = 5000;
	int multiplier = 10;
	static TLSPool<Node, 0, false> _tlsLFQNodePool;


	short tailCount = 0;
	Node* queueID;
	static int64 GID;


	static const int debugSize =1000;
	long long debugIndex = 0;
	//debugData<T> debug[debugSize];


	long tryEnqueueCount = 0;
	Node* _tail = nullptr;
};
template <typename T>
TLSPool<typename TLSLockFreeQueue<T>::Node, 0, false> typename TLSLockFreeQueue<T>::_tlsLFQNodePool(100, 1000);

template <typename T>
int64 TLSLockFreeQueue<T>::GID = 0;