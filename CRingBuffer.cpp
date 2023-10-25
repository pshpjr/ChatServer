
#include "stdafx.h"
#include "CRingBuffer.h"

#include "CSerializeBuffer.h"


CRingBuffer::CRingBuffer(int size) :BUFFER_SIZE(size), ALLOC_SIZE(BUFFER_SIZE + 10), _buffer(new char[ALLOC_SIZE]), _front(0), _rear(0)
{
	memset(_buffer + BUFFER_SIZE, 0, 10);
	InitializeCriticalSection(&_cs);
}

CRingBuffer::~CRingBuffer()
{
	delete[] _buffer;
}

int CRingBuffer::Size() const
{
	int rear = _rear;
	int front = _front;

	if (rear >= front)
	{
		return rear - front;
	}
	else
	{
		return BUFFER_SIZE - (front - rear);
	}

}

int CRingBuffer::DirectEnqueueSize() const
{
	int rear = _rear;
	int front = _front;

	//front는 변할 수 있지만 rear는 변하지 않음. 
	if ((rear + 1) % BUFFER_SIZE == front)
	{
		return 0;
	}

	//if문 탐지할 때 front와 들어가서 front 값이 달라질 수 있음.
	//front는 증가만 하지만 rear와 front가 둘 다 버퍼 끝자락에서 있을 때 front가 증가해 rear보다 작아질 수 있음.
	//이 경우 result가 음수 나올 수 있음.
	if (rear <= ((front + BUFFER_SIZE - 1) % BUFFER_SIZE))
	{
		int result = (front + BUFFER_SIZE - 1) % BUFFER_SIZE - rear;
		ASSERT_CRASH(result > 0, "Out of Case");
		return result;
	}


	if (rear >= front)
	{
		int result = BUFFER_SIZE - rear;

		ASSERT_CRASH(result > 0, "Out of Case");

		return result;
	}

	ASSERT_CRASH(false, "Out of Case");

	return 0;
}

int CRingBuffer::DirectDequeueSize() const
{
	int rear = _rear;
	int front = _front;
	if (rear >= front)
	{
		return rear - front;
	}
	else
	{
		return BUFFER_SIZE - front;
	}
}


int CRingBuffer::Peek(char* dst, int size) const
{
	if (_front == _rear)
		return 0;

	int peekSize = size;
	//size는 늘어나기만 함. 
	if (Size() < size)
	{
		DebugBreak();
		peekSize = Size();
	}

	if (DirectDequeueSize() >= peekSize)
	{
		memcpy_s(dst, peekSize, _buffer + _front, peekSize);
	}
	else
	{
		int deqSize = DirectDequeueSize();

		memcpy_s(dst, deqSize, _buffer + _front, deqSize);
		memcpy_s(dst + deqSize, peekSize - deqSize, _buffer, peekSize - deqSize);
	}

	return peekSize;
}


int CRingBuffer::Enqueue(char* data, int dataSize)
{
	//버퍼 꽉 참.
	if ((_rear + 1) % BUFFER_SIZE == _front)
		return 0;

	//이 아래는 버퍼가 가득 차지 않았다고 가정. 버퍼에 있는 데이터 크기는 줄어들기만 함. 

	int insertSize = dataSize;

	//direcEnqueueSize 함수도 항상 증가함. 변경되어도 상관 없음.

	//freeSize 함수의 결과는 항상 증가함. 변경되어도 상관 없음.
	//여기서 음수가 나왔다 : 
	int freeRet = FreeSize();
	if (freeRet < dataSize)
	{
		DebugBreak();
		insertSize = FreeSize();
	}

	//rear 나만 건드림
	if (DirectEnqueueSize() >= insertSize)
	{
		memcpy_s(_buffer + _rear, insertSize, data, insertSize);
	}
	else
	{
		int directEnqueueSize = DirectEnqueueSize();
		memcpy_s(_buffer + _rear, directEnqueueSize, data, directEnqueueSize);
		memcpy_s(_buffer, insertSize - directEnqueueSize, data + directEnqueueSize, insertSize - directEnqueueSize);
	}

	//rear는 항상 나만 건드림
	_rear = (_rear + insertSize) % BUFFER_SIZE;



	return insertSize;
}


bool CRingBuffer::DequeueCBuffer(int count)
{
	int iter = _front;
	_front = (_front + (count*sizeof(void*))) % BUFFER_SIZE;

	for (int i = 0; i < count; ++i)
	{
		auto ptr = *(CSerializeBuffer**)(_buffer + iter);
		ptr->Clear();
		iter = (iter + sizeof(void*)) % BUFFER_SIZE;
	}
	return true;
}

bool CRingBuffer::EnqueueCBuffer(CSerializeBuffer* buffer)
{
	//버퍼 꽉 참.
	if ((_rear + 1) % BUFFER_SIZE == _front)
		return 0;
	

	int dataSize = sizeof(buffer);
	int insertSize = dataSize;

	int freeRet = FreeSize();
	if (freeRet < dataSize)
	{
		DebugBreak();
		insertSize = FreeSize();
	}

	//rear 나만 건드림
	*(CSerializeBuffer**)(_buffer+_rear) =buffer;


	//rear는 항상 나만 건드림
	_rear = (_rear + insertSize) % BUFFER_SIZE;



	return insertSize;

}


int CRingBuffer::Dequeue(int deqSize)
{
	if (_front == _rear)
		return 0;

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

