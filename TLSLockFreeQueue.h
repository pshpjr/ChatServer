#pragma once
#include "TLSPool.h"
#include "Types.h"


template <typename T>
class TlsLockFreeQueue
{
	struct Node
	{
		Node* next = nullptr;
		T data {};
		Node() = default;
		explicit Node(const T& data) : next(nullptr), data(data) {}
	};

public:
	TlsLockFreeQueue() : _queueId(( Node* ) InterlockedIncrement64(&_gid))
	{
		Node* dummy = _tlsLfqNodePool.Alloc();

		dummy->next = _queueId;
		_head = dummy;
		_tail = dummy;
	}

	void Enqueue(const T& data)
	{
		Node* allocNode; {
			allocNode = _tlsLfqNodePool.Alloc();
		}

		Node* newNode = allocNode;
		newNode->data = data;
		newNode->next = _queueId;

		auto headCount = InterlockedIncrement16(&_tailCount);
		Node* newTail = ( Node* ) ( ( unsigned long long )newNode | ( ( unsigned long long )( headCount ) ) << 47 );
		{
			while ( true )
			{
				Node* tail = _tail;
				Node* tailNode = ( Node* ) ( ( unsigned long long )tail & lock_free_data::pointerMask );

				if ( tailNode->next == _queueId )
				{
					if ( InterlockedCompareExchangePointer(( PVOID* ) &tailNode->next, newTail, ( PVOID* ) _queueId) == _queueId )
					{

						if ( InterlockedCompareExchangePointer(( PVOID* ) &_tail, newTail, tail) == tail )
						{
						}

						break;
					}
				}
				//tail의 next가 _queueId가 아니면 경우의 수는 2가지
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
		//debug[index].newHead = (unsigned long long)((Node*)((unsigned long long)newNode & lock_free_data::pointerMask));
		//debug[index].data = newNode->data;
	}

	inline bool Dequeue(T& data)
	{
		if ( _size == 0 )
		{
			return false;
		}

		bool find = false;

		while ( true )
		{

			//내가 deq 하려고 했는데 남이 해버리면
			//나는 이 지점에 왔을 때 head next가 널이 되어버림.
			Node* head = _head;
			Node* headNode = ( Node* ) ( ( unsigned long long )head & lock_free_data::pointerMask );

			Node* next = headNode->next;
			Node* nextNode = ( Node* ) ( ( unsigned long long )next & lock_free_data::pointerMask );

			if ( nextNode < ( Node* ) 64000 )
				return false;

			//auto debugCount = InterlockedIncrement64(&debugIndex);
			//auto index = debugCount % debugSize;

			//debug[index].threadID = std::this_thread::get_id();
			//debug[index].type = IoTypes::TryDequeue;

			//debug[index].oldHead = (unsigned long long)headNode;
			//debug[index].newHead = (unsigned long long)nextNode;
			//debug[index].data = nextNode->data;

			data = nextNode->data;

			if ( InterlockedCompareExchangePointer(( PVOID* ) &_head, next, head) == head )
			{
				Node* tail = _tail;
				Node* tailNode = ( Node* ) ( ( unsigned long long )tail & lock_free_data::pointerMask );

				if ( tail == head )
				{
					//내가 뺀 애가 tail이면 풀에 넣기 전에 수정해줘야 함. 아니면 꼬임.
					InterlockedCompareExchangePointer(( PVOID* ) &_tail, tailNode->next, tail);
				}


				_tlsLfqNodePool.Free(headNode);
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

	int _initSize = 5000;
	int _multiplier = 10;
	static TlsPool<Node, 0, false> _tlsLfqNodePool;


	short _tailCount = 0;
	Node* _queueId;
	static int64 _gid;


	static constexpr int DEBUG_SIZE =1000;
	long long _debugIndex = 0;
	//debugData<T> debug[debugSize];


	long tryEnqueueCount = 0;
	Node* _tail = nullptr;
};
template <typename T>
TlsPool<typename TlsLockFreeQueue<T>::Node, 0> typename TlsLockFreeQueue<T>::_tlsLfqNodePool(10000, 10);

template <typename T>
int64 TlsLockFreeQueue<T>::_gid = 0;