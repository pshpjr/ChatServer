#pragma once

template <typename T>
class LockFreeFixedQueue
{
	struct Node
	{
		char isUsed = 0;
		T data;
	};
public:
	constexpr LockFreeFixedQueue(int size) : indexMask(size - 1), buffer(size)
	{
	}

	LockFreeFixedQueue(LockFreeFixedQueue const& other) = delete;
	LockFreeFixedQueue(LockFreeFixedQueue&& other) = delete;

	LockFreeFixedQueue& operator = (LockFreeFixedQueue const& other) = delete;
	LockFreeFixedQueue& operator = (LockFreeFixedQueue&& other) = delete;

	~LockFreeFixedQueue() = default;

	int Size() const
	{
		return abs(( headIndex & indexMask ) - ( tailIndex & indexMask ));

	}

	bool Enqueue(const T& data)
	{
		long tail = InterlockedIncrement(&tailIndex) - 1;

		if ( buffer[tail & indexMask].isUsed == true )
		{
			DebugBreak();
		}

		buffer[tail & indexMask].data = data;
		if ( InterlockedExchange8(&buffer[tail & indexMask].isUsed, true) == true )
			DebugBreak();
		return true;

	}

	bool Dequeue(T& data)
	{
		long head;
		for ( ;; )
		{
			head = headIndex;

			if ( buffer[head & indexMask].isUsed == false )
				return false;

			if ( InterlockedCompareExchange(&headIndex, head + 1, head) == head )
			{
				break;
			}
		}
		data = buffer[head & indexMask].data;

		if ( InterlockedExchange8(&buffer[head & indexMask].isUsed, false) == false )
			DebugBreak();
		return true;
	}


	alignas( 64 ) long headIndex = 0;
	alignas( 64 ) long tailIndex = 0;

	int indexMask;
	vector<Node> buffer;
public:

};

