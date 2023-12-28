#pragma once

#include <winnt.h>
#include "MultiThreadObjectPool.h"
#include "LockFreeData.h"

template <typename T>
class LockFreeStack
{


	struct Node
	{
		T data {};
		Node* next = nullptr;
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

		for ( ;;)
		{
			Node* top = _top;
			node->next = ( Node* ) ( ( long long ) top & lock_free_data::pointerMask );

			Node* newTop = ( Node* ) ( ( unsigned long long )( node ) | ( ( long long ) top + lock_free_data::indexInc & lock_free_data::indexMask ) );

			if ( InterlockedCompareExchange64(( __int64* ) &_top, ( __int64 ) newTop, ( __int64 ) top) == ( __int64 ) top )
			{
				InterlockedIncrement(&objectsInPool);

				break;
			}
		}
	}

	bool Pop(T& data)
	{
		for ( ;; )
		{
			Node* top = _top;

			Node* topNode = ( Node* ) ( ( unsigned long long )top & lock_free_data::pointerMask );
			if ( topNode == nullptr )
			{
				return false;
			}



			data = topNode->data;
			if ( InterlockedCompareExchange64(( __int64* ) &_top, ( long long ) topNode->next | ( long long ) top & lock_free_data::indexMask, ( __int64 ) top) == ( __int64 ) top )
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

	Node* _top=nullptr;

	MultiThreadObjectPool<Node, false> _pool;
};
