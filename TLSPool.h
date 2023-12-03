#pragma once
#include "Windows.h"
#include "SingleThreadObjectPool.h"
#include "Profiler.h"
static constexpr int TLS_POOL_INITIAL_SIZE = 5'000;
static constexpr int GLOBAL_POOL_INITIAL_SIZE = TLS_POOL_INITIAL_SIZE*10;


template <typename T, int dataId, bool usePlacement = false>
class TLSPool
{
public:
	using poolType = SingleThreadObjectPool<T, dataId, usePlacement>;
	using Node = typename poolType::Node;


	TLSPool():poolID(InterlockedIncrement(&GPoolID))
	{

		InitializeCriticalSection(&_cs);
		localPoolTlsIndex = TlsAlloc();

		_pooledNodeSize = GLOBAL_POOL_INITIAL_SIZE;
		
		for (int i = 0; i < _pooledNodeSize; ++i)
		{
			Node* newNode = createNode();

			newNode->_tail = _top;
			_top = newNode;
		}
	}

	int GetGPoolSize() const { return _pooledNodeSize; }
	int GetGPoolEmptyCount() const { return _poolEmptyCount; }
	int GetAcquireCount() const { return _acquireCount; }
	int GetRelaseCount() const { return _releaseCount; }

	Node* createNode()
	{
		Node* newNode = (Node*)malloc(sizeof(Node));
		newNode->_tail = nullptr; 

#ifdef TLSPoolDebug
		allocked.push_back(newNode);
#endif
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
		int i = 0;
		Node* ret = nullptr;
		EnterCriticalSection(&_cs);
		_acquireCount++;

		Node* newTop = _top;
		if(_pooledNodeSize < size)
		{
			PRO_BEGIN("ACQUIRE_NEW")
			for (int i = 0; i < size*2; ++i)
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
			_pooledNodeSize += size;
			_poolEmptyCount++;
			newTop->_tail;
		}
		else
		{
			PRO_BEGIN("ACQUIRE_INPOOL")
			ret = _top;

			for (int i = 0; i < size-1; ++i)
			{
				newTop = newTop->_tail;
			}
			_top = newTop->_tail;
			newTop->_tail = nullptr;

			_pooledNodeSize -= size;
			newTop->_tail;
		}


		LeaveCriticalSection(&_cs);

		return ret;
	}


	void ReleaseNode(Node* Head,Node* tail,int size)
	{
		EnterCriticalSection(&_cs);


		_releaseCount++;

		tail->_tail = _top;



		_top = Head;

		_pooledNodeSize += size;



		LeaveCriticalSection(&_cs);
	}


	T* Alloc()
	{
		PRO_BEGIN("TLSPOOL_ALLOC")
		poolType* pool = (poolType*)TlsGetValue(localPoolTlsIndex);

		if (pool == nullptr)
		{
			pool = new poolType(TLS_POOL_INITIAL_SIZE);
			TlsSetValue(localPoolTlsIndex, pool);
		}

		if(pool->GetObjectCount() == 0)
		{
			PRO_BEGIN("TLSPOOL_ALLOC_ACQUIRE")
			pool->_top = AcquireNode(TLS_POOL_INITIAL_SIZE);
			pool->_objectCount = TLS_POOL_INITIAL_SIZE;

		}
		PRO_BEGIN("TLSPOOL_ALLOC_PoolAlloc")
		return pool->Alloc();
	}

	void Free(T* data)
	{
		//PRO_BEGIN("FREE")
		poolType* pool = (poolType*)TlsGetValue(localPoolTlsIndex);

		if (pool == nullptr)
		{
			pool = new poolType(TLS_POOL_INITIAL_SIZE);
			TlsSetValue(localPoolTlsIndex, pool);
		}

		pool->Free(data);

		if (pool->GetObjectCount() > TLS_POOL_INITIAL_SIZE*2)
		{
			//PRO_BEGIN("FREE_RELEASE")
			Node* releaseHead = pool->_top;

			Node* releaseTail = pool->_top;

			for (int i = 0; i < TLS_POOL_INITIAL_SIZE-1; ++i) {
				releaseTail = releaseTail->_tail;
			}

			Node* newHead = releaseTail->_tail;
			releaseTail->_tail = nullptr;
			pool->_top = newHead;

			ReleaseNode(releaseHead, releaseTail, TLS_POOL_INITIAL_SIZE);

			pool->_objectCount -= TLS_POOL_INITIAL_SIZE;

		}
	}


private:
	CRITICAL_SECTION _cs;



	inline static DWORD localPoolTlsIndex = 0;
	Node* _top = nullptr;
	int _pooledNodeSize = 0;
	inline static long GPoolID;
	long poolID = 0;
	const int _localPoolSize = TLS_POOL_INITIAL_SIZE;
	const int _globalPoolSize = GLOBAL_POOL_INITIAL_SIZE;

	unsigned long _acquireCount = 0;
	unsigned long _releaseCount = 0;

	unsigned int _poolEmptyCount = 0;
#ifdef TLSPoolDebug
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
	list<Node*> allocked;
#endif
};

#ifndef USE_TLS_POOL

#define USE_TLS_POOL(CLASS) 	friend TLSPool<CLASS, 0, false>; friend SingleThreadObjectPool<CLASS, 0, false>;


#endif