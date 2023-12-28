
#include "stdafx.h"
#include "CRingBuffer.h"



CRingBuffer::CRingBuffer(const int size) :BUFFER_SIZE(size), ALLOC_SIZE(BUFFER_SIZE + 10), _buffer(new char[ALLOC_SIZE]), _front(0), _rear(0)
{
	memset(_buffer + BUFFER_SIZE, 0, 10);
}

CRingBuffer::~CRingBuffer()
{
	delete[] _buffer;
}

//front가 맨 앞이고 rear가 맨 끝인 상황 처리 해야 함. 
int CRingBuffer::Size() const
{
	const int rear = _rear;
	const int front = _front;

	if (rear >= front)
	{
		return rear - front;
	}

	return BUFFER_SIZE - (front - rear);
	
}
int CRingBuffer::DirectEnqueueSize() const
{
	const int rear = _rear;
	const int front = _front;

	//front는 변할 수 있지만 rear는 변하지 않음. 
	if ((rear + 1) % BUFFER_SIZE == front)
	{
		return 0;
	}

	//if문 탐지할 때 front와 들어가서 front 값이 달라질 수 있음.
	//front는 증가만 하지만 rear와 front가 둘 다 버퍼 끝자락에서 있을 때 front가 증가해 rear보다 작아질 수 있음.
	//이 경우 result가 음수 나올 수 있음.
	if (rear < front )
	{
		const int result = front - rear - 1;
		ASSERT_CRASH(result > 0, "Out of Case");
		return result;
	}
	if (rear >= front)
	{
		int result;
		if (front == 0)
		{
			result = BUFFER_SIZE - rear - 1;
		}
		else
		{
			result = BUFFER_SIZE - rear;
		}

		ASSERT_CRASH(result > 0, "Out of Case");

		return result;
	}

	ASSERT_CRASH(false, "Out of Case");

	return 0;
}
//int CRingBuffer::DirectEnqueueSize() const
//{
//	int rear = _rear;
//	int front = _front;
//
//	//front는 변할 수 있지만 rear는 변하지 않음. 
//	if ((rear + 1) % BUFFER_SIZE == front)
//	{
//		return 0;
//	}
//
//	//if문 탐지할 때 front와 들어가서 front 값이 달라질 수 있음.
//	//front는 증가만 하지만 rear와 front가 둘 다 버퍼 끝자락에서 있을 때 front가 증가해 rear보다 작아질 수 있음.
//	//이 경우 result가 음수 나올 수 있음.
//	if (rear <= ((front + BUFFER_SIZE - 1) % BUFFER_SIZE))
//	{
//		int result = (front + BUFFER_SIZE - 1) % BUFFER_SIZE - rear;
//		ASSERT_CRASH(result > 0, "Out of Case");
//		return result;
//	}
//	if (rear >= front)
//	{
//		int result;
//		if (front == 0)
//			result = BUFFER_SIZE - rear - 1;
//		else
//			result = BUFFER_SIZE - rear;
//
//		ASSERT_CRASH(result > 0, "Out of Case");
//
//		return result;
//	}
//
//	ASSERT_CRASH(false, "Out of Case");
//
//	return 0;
//}

int CRingBuffer::DirectDequeueSize() const
{
	const int rear = _rear;
	const int front = _front;
	if (rear >= front)
	{
		return rear - front;
	}
	return BUFFER_SIZE - front;
}


int CRingBuffer::Dequeue(const int deqSize)
{
	if (_front == _rear)
	{
		return 0;
	}

	//이후 dequeue 할 내용이 있다고 가정함. 

	int size = deqSize;

	//size 함수는 늘어나기만 함. if 이후 변경되어도 상관 없음. 
	if (deqSize > Size())
	{
		DebugBreak();
		size = Size();
	}

	_front = (_front + size) % BUFFER_SIZE;


	return size;
}

int CRingBuffer::Peek(char* dst, const int size) const
{
	if ( _front == _rear )
	{
		return 0;
	}

	int peekSize = size;
	//size는 늘어나기만 함. 
	if ( Size() < size )
	{
		DebugBreak();
		peekSize = Size();
	}

	if ( DirectDequeueSize() >= peekSize )
	{
		memcpy_s(dst, peekSize, _buffer + _front, peekSize);
	}
	else
	{
		const int deqSize = DirectDequeueSize();

		memcpy_s(dst, deqSize, _buffer + _front, deqSize);
		memcpy_s(dst + deqSize, peekSize - deqSize, _buffer, peekSize - deqSize);
	}

	return peekSize;
}