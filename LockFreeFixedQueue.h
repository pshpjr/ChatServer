#pragma once

template <typename T,int BufferSize>
class LockFreeFixedQueue
{
	static_assert( ( BufferSize & BufferSize - 1 ) == 0 );

	struct Node
	{
		char isUsed = 0;
		T data = T();
	};
public:
	constexpr LockFreeFixedQueue() : indexMask(BufferSize - 1), buffer(BufferSize)
	{

	}

	LockFreeFixedQueue(LockFreeFixedQueue const& other) = delete;
	LockFreeFixedQueue(LockFreeFixedQueue&& other) = delete;

	LockFreeFixedQueue& operator = (LockFreeFixedQueue const& other) = delete;
	LockFreeFixedQueue& operator = (LockFreeFixedQueue&& other) = delete;

	~LockFreeFixedQueue() = default;

	int Size() const
	{
		auto head = headIndex & indexMask;
		auto tail = tailIndex & indexMask;

		if ( tail >= head )
		{
			return tail - head;
		}

		return BufferSize - ( head - tail );
	}

	bool Enqueue(const T& data)
	{
		const long tail = InterlockedIncrement(&tailIndex) - 1;

		if ( buffer[tail & indexMask].isUsed == true )
		{
			DebugBreak();
		}

		buffer[tail & indexMask].data = data;
		if ( InterlockedExchange8(&buffer[tail & indexMask].isUsed, true) == true )
		{
			DebugBreak();
		}
		return true;

	}

	bool Dequeue(T& data)
	{
		long head;
		for ( ;; )
		{
			head = headIndex;

			if ( buffer[head & indexMask].isUsed == false )
			{
				return false;
			}

			if ( InterlockedCompareExchange(&headIndex, head + 1, head) == head )
			{
				break;
			}
		}
		data = buffer[head & indexMask].data;

		if ( InterlockedExchange8(&buffer[head & indexMask].isUsed, false) == false )
		{
			DebugBreak();
		}
		return true;
	}


	alignas( 64 ) long headIndex = 0;
	alignas( 64 ) long tailIndex = 0;

	int indexMask;
	vector<Node> buffer;
public:

};

