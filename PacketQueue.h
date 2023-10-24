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


	void Enqueue(ContentJob& job) 
	{
		EnterCriticalSection(&_lock);
		_queue[_enqueueIndex].push(job);
		LeaveCriticalSection(&_lock);
	}

	bool Dequeue(ContentJob& out) 
	{
		if (_queue[_enqueueIndex].empty()) {
			return false;
		}

		out = _queue[_enqueueIndex].front();
		_queue[_enqueueIndex].pop();
	}

private:
	int _enqueueIndex = 0;
	int _dequeueIndex = 1;
	queue<ContentJob> _queue[2];

	CRITICAL_SECTION _lock;

};

