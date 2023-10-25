#pragma once
#include "ContentJob.h"
class PacketQueue
{
public:
	PacketQueue()
	{
		InitializeCriticalSection(&_lock);
	}

	void Swap() 
	{	
		EnterCriticalSection(&_lock);

		_dequeueIndex = _enqueueIndex;
		_enqueueIndex ^= 1;

		LeaveCriticalSection(&_lock);
	}


	void Enqueue(ContentJob* job) 
	{
		EnterCriticalSection(&_lock);
		_queue[_enqueueIndex].push(job);
		LeaveCriticalSection(&_lock);
	}

	ContentJob* Dequeue()
	{
		if (_queue[_dequeueIndex].empty()) {
			return nullptr;
		}

		auto ret = _queue[_dequeueIndex].front();
		_queue[_dequeueIndex].pop();
		return ret;
	}

private:
	int _enqueueIndex = 0;
	int _dequeueIndex = 1;
	queue<ContentJob*> _queue[2];

	CRITICAL_SECTION _lock;

};

