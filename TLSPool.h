#pragma once
#include "Windows.h"
#include "SingleThreadObjectPool.h"

static constexpr int TLS_POOL_INITIAL_SIZE = 2000;
static constexpr int GLOBAL_POOL_INITIAL_SIZE = TLS_POOL_INITIAL_SIZE*10;


template <typename T, int dataId, bool usePlacement = false>
class TLSPool
{

public:
	using poolType = SingleThreadObjectPool<T, dataId, usePlacement>;
	using Node = typename poolType::Node;

	int checkNode(Node* top) {
		auto count = 0;
		auto start = top;

		while (start != nullptr) {
			start = start->_tail;
			count++;
		}
		return count;
	}

	TLSPool():poolID(InterlockedIncrement(&GPoolID))
	{

		InitializeCriticalSection(&_cs);
		localPoolTlsIndex = TlsAlloc();

		_pooledNodeCount = GLOBAL_POOL_INITIAL_SIZE;

		for (int i = 0; i < _pooledNodeCount; ++i)
		{
			Node* newNode = createNode();

			newNode->_tail = _top;
			_top = newNode;
		}
	}


	Node* createNode()
	{
		Node* newNode = (Node*)malloc(sizeof(Node));
		newNode->_head = nullptr;
		newNode->_tail = nullptr; 


		if constexpr (!usePlacement)
			new (&newNode->_data) T();
		return newNode;
	}

	/**
	 * \brief 노드 쓰는 각종 작업에서 리턴하는 건 뭉치의 가장 처음 노드. 
	 * \return 
	 */
	Node* AcquireNode(int size)
	{
		EnterCriticalSection(&_cs);

		int i = 0;

		Node* newTop = _top;
		Node* ret = nullptr;


		if(_pooledNodeCount < size)
		{
			for (int i = 0; i < size; ++i)
			{
				Node* newNode = createNode();

				newNode->_tail = newTop;
				newTop = newNode;
			}
			_top = newTop;

			for (int i = 0; i < size; ++i)
			{
				Node* newNode = createNode();

				newNode->_tail = ret;
				ret = newNode;
			}
			//_pooledNodeCount += size;
		}
		else
		{
			ret = _top;

			for (int i = 0; i < size-1; ++i)
			{
				newTop = newTop->_tail;
			}
			_top = newTop->_tail;
			newTop->_tail = nullptr;

			_pooledNodeCount -= size;
		}


		LeaveCriticalSection(&_cs);

		return ret;
	}


	void ReleaseNode(Node* Head,Node* tail,int size)
	{
		EnterCriticalSection(&_cs);

		tail->_tail = _top;
		_top = Head;

		_pooledNodeCount += size;



		LeaveCriticalSection(&_cs);
	}


	T* Alloc()
	{
		InterlockedIncrement64(&allocCount);
		poolType* pool = (poolType*)TlsGetValue(localPoolTlsIndex);

		if (pool == nullptr)
		{
			pool = new poolType(TLS_POOL_INITIAL_SIZE);
			TlsSetValue(localPoolTlsIndex, pool);
		}

		if(pool->GetObjectCount() == 0)
		{
			pool->_top = AcquireNode(TLS_POOL_INITIAL_SIZE);
			pool->_objectCount = TLS_POOL_INITIAL_SIZE;


		}

		return pool->Alloc();
	}

	void Free(T* data)
	{
		InterlockedIncrement64(&freeCount);
		poolType* pool = (poolType*)TlsGetValue(localPoolTlsIndex);

		if (pool == nullptr)
		{
			pool = new poolType(TLS_POOL_INITIAL_SIZE);
			TlsSetValue(localPoolTlsIndex, pool);
		}

		pool->Free(data);

		if (pool->GetObjectCount() > TLS_POOL_INITIAL_SIZE*2)
		{
			Node* releaseHead = pool->_top;

			Node* releaseTail = pool->_top;

			for (int i = 0; i < TLS_POOL_INITIAL_SIZE-1; ++i) {
				releaseTail = releaseTail->_tail;
			}

			Node* newHead = releaseTail->_tail;

			pool->_top = newHead;

			ReleaseNode(releaseHead, releaseTail, TLS_POOL_INITIAL_SIZE);

			pool->_objectCount -= TLS_POOL_INITIAL_SIZE;

		}
	}


private:
	CRITICAL_SECTION _cs;

	inline static DWORD localPoolTlsIndex = 0;
	Node* _top = nullptr;
	int _pooledNodeCount = 0;
	inline static long GPoolID;
	long poolID = 0;
	const int _localPoolSize = TLS_POOL_INITIAL_SIZE;
	const int _globalPoolSize = GLOBAL_POOL_INITIAL_SIZE;
	long long freeCount = 0;
	long long allocCount = 0;

	struct Debug
	{
		int threadID;
		int type;
		int globalPoolSize;
		long long globalPoolAddr;
		int localPoolSize;
		long long localPoolAddr;
	};
	long debugIndex = 0;
	Debug debug[1000];
};

#ifndef USE_TLS_POOL

#define USE_TLS_POOL(CLASS) 	friend TLSPool<CLASS, 0, false>; friend SingleThreadObjectPool<CLASS, 0, false>;


#endif