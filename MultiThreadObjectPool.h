#pragma once
#include <cstdlib>
#include <winnt.h>
#include "LockFreeData.h"

template <typename T, bool usePlacement = false>
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

	MultiThreadObjectPool(int baseAllocSize) :_count(baseAllocSize)
	{
		for ( int i = 0; i < baseAllocSize; ++i )
		{
			auto newNode = createNode();
			newNode->next = _top;
			_top = newNode;
		}
	}
	~MultiThreadObjectPool()
	{
		Node* p = (Node*)((long long)_top & ::pointerMask);
		for ( ; p != nullptr;)
		{
			Node* next = p->next;
			if constexpr ( !usePlacement )
			{
				p->~Node();
			}
			free(p);
			p = next;
		}
	}

	long Size() const { return _count; }

	inline T* Alloc()
	{
		Node* retNode;

		for ( ;;)
		{
			Node* top = _top;

			Node* topNode = ( Node* ) ( ( unsigned long long )top & ::pointerMask );
			if ( topNode == nullptr )
			{
				break;
			}

			if ( InterlockedCompareExchange64(( __int64* ) &_top,  (long long )topNode->next | (long long)top & ::indexMask, ( __int64 ) top) == ( __int64 ) top )
			{
				retNode = ( Node* ) ( ( unsigned long long )top & ::pointerMask );

				if constexpr ( usePlacement )
				{
					new( &retNode->data ) T();
				}
				InterlockedDecrement(&_count);
				return &retNode->data;
			}
		}

		retNode = createNode();
		return &retNode->data;
	}

	inline void Free(T* data)
	{

		if ( usePlacement )
		{
			data->~T();
		}
		Node* node = ( Node* ) ( ( unsigned long long )data - offsetof(Node, data) );
		//Node* newTop = ( Node* ) ( ( unsigned long long )( node ) | ( ( unsigned long long )( InterlockedIncrement16(&topCount) ) << 47 ) );


		for ( ;;)
		{
			auto top = _top;
			node->next = (Node*) ((long long)top & ::pointerMask);
			Node* newTop = ( Node* ) ( ( unsigned long long )( node ) | ( ((long long)top + ::indexInc) & ::indexMask  ));

			if ( InterlockedCompareExchange64(( __int64* ) &_top, ( __int64 ) newTop, ( __int64 ) top) == ( __int64 ) top )
			{
				InterlockedIncrement(&_count);
				break;
			}
		}


	}

private:
	inline Node* createNode()
	{
		Node* node = ( Node* ) malloc(sizeof(Node));
		node->next = nullptr;
		if constexpr ( !usePlacement )
		{
			new( &node->data ) T();
		}
		return node;
	}

private:
	Node* _top;

	short topCount = 0;
	long _count = 0;
};
