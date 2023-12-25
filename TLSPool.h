#pragma once
#include "SingleThreadObjectPool.h"
#include "Profiler.h"
#include "LockFreeStack.h"

const int TLS_POOL_INITIAL_SIZE = 5'000;
const int GLOBAL_POOL_MULTIPLIER = 10;

//#define TLS_LEAK_DEBUG
//#define TLS_DEBUG


template <typename T, int dataId, bool usePlacement = false>
class TLSPool
{

	using poolType = SingleThreadObjectPool<T, dataId, usePlacement>;
	using Node = typename poolType::Node;
public:


	int AllockedCount() { return ( _acquireCount - _releaseCount ) * _localPoolSize; }

	std::list<Node*> allocked;



	struct controlData
	{
		Node* front;
		Node* end;
	};

	TLSPool(int localPoolSize, int globalPoolMultiplier):
		poolID(InterlockedIncrement(&GPoolID)),
		_localPoolSize(localPoolSize),
		_globalPoolMultiplier(globalPoolMultiplier)
	{
#ifdef TLS_LEAK_DEBUG
		InitializeCriticalSection(&_debugLock);
#endif

		InitializeCriticalSection(&_cs);
		localPoolTlsIndex = TlsAlloc();

		_pooledNodeSize = _globalPoolMultiplier;
		
		for ( int j = 0; j < _globalPoolMultiplier; j++ )
		{

			_nextBucket.Push(makeControl());
			
		}
	}

	int GetGPoolSize() const { return _pooledNodeSize; }
	int GetGPoolEmptyCount() const { return _poolEmptyCount; }
	int GetAcquireCount() const { return _acquireCount; }
	int GetRelaseCount() const { return _releaseCount; }

	Node* createNode()
	{
		Node* newNode = (Node*)malloc(sizeof(Node));
		__analysis_assume(newNode != nullptr);
		newNode->_tail = nullptr; 

#ifdef TLS_LEAK_DEBUG
		EnterCriticalSection(&_debugLock);
		allocked.push_back(newNode);
		LeaveCriticalSection(&_debugLock);
#endif

		if constexpr (!usePlacement)
			new (&newNode->_data) T();
		return newNode;
	}

	/**
	 * \brief 노드 쓰는 각종 작업에서 리턴하는 건 뭉치의 가장 처음 노드. 
	 * \return 
	 */
	controlData AcquireNode(int size)
	{
		InterlockedIncrement(&_acquireCount);


		controlData data;

		if ( !_nextBucket.Pop(data) )
		{

			auto cb1 = makeControl();
			auto retCb = makeControl();

			_nextBucket.Push(cb1);

			InterlockedIncrement(&_poolEmptyCount);
			InterlockedAdd(&_pooledNodeSize, size);
			return retCb;
		}
		InterlockedAdd(&_pooledNodeSize, -size);

		return data;
	}


	void ReleaseNode(Node* Head,Node* tail,int size)
	{
		controlData cb {};
		cb.front = Head;
		cb.end = tail;

		_nextBucket.Push(cb);

		InterlockedIncrement(&_releaseCount);
		InterlockedAdd(&_pooledNodeSize, size);
	}


	inline T* Alloc()
	{
		poolType* pool = (poolType*)TlsGetValue(localPoolTlsIndex);

		if (pool == nullptr)
		{
			pool = new poolType(0);
			TlsSetValue(localPoolTlsIndex, pool);
		}

		if(pool->GetObjectCount() == 0)
		{
			auto cb = AcquireNode(1);

			pool->_top = cb.front;
			pool->_objectCount = _localPoolSize;
		}
		return pool->Alloc();
	}

	inline void Free(T* data)
	{
		//PRO_BEGIN("FREE")
		poolType* pool = (poolType*)TlsGetValue(localPoolTlsIndex);

		if (pool == nullptr)
		{
			pool = new poolType(0);
			TlsSetValue(localPoolTlsIndex, pool);
		}

		pool->Free(data);

		if (pool->GetObjectCount() > _localPoolSize *2)
		{
			//PRO_BEGIN("FREE_RELEASE")
			Node* releaseHead = pool->_top;

			Node* releaseTail = pool->_top;

			for (int i = 0; i < _localPoolSize -1; ++i) {
				releaseTail = releaseTail->_tail;
			}

			Node* newHead = releaseTail->_tail;
			releaseTail->_tail = nullptr;


			pool->_top = newHead;

			ReleaseNode(releaseHead, releaseTail, 1);

			pool->_objectCount -= _localPoolSize;

		}
	}

	controlData makeControl()
	{
		Node* newTop = createNode();

		controlData data = {};
		data.end = newTop;

		for ( int i = 0; i < _localPoolSize -1; ++i )
		{
			Node* newNode = createNode();

			newNode->_tail = newTop;
			newTop = newNode;
		}

		data.front = newTop;

		return data;
	}

private:
	CRITICAL_SECTION _cs;

	LockFreeStack<controlData> _nextBucket;

	inline static DWORD localPoolTlsIndex = 0;
	Node* _top = nullptr;
	long _pooledNodeSize = 0;
	inline static long GPoolID;
	long poolID = 0;
	int _localPoolSize;
	int _globalPoolMultiplier;

	long _acquireCount = 0;
	long _releaseCount = 0;

	unsigned int _poolEmptyCount = 0;
#ifdef TLS_DEBUG
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

#endif

#ifdef TLS_LEAK_DEBUG

	CRITICAL_SECTION _debugLock;

#endif
};

#ifndef USE_TLS_POOL

#define USE_TLS_POOL(CLASS) 	friend TLSPool<CLASS, 0, false>; friend SingleThreadObjectPool<CLASS, 0, false>;

#endif