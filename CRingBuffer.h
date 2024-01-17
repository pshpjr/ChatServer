#pragma once

class CRingBuffer
{
public:
	//버퍼 사이즈는 10의 거듭제곱이어야 한다. 
	CRingBuffer(int size = 10000);

	~CRingBuffer();


	/**
	 * \
	 * \return 사용중인 데이터의 크기
	 */
	int Size() const;


	/**
	 * \brief 한 번에 넣을 수 있는 데이터의 크기
	 * \return
	 */
	int DirectEnqueueSize() const;
	/**
	 * \brief
	 * \return 한 번에 뺄 수 있는 데이터의 크기
	 */
	int DirectDequeueSize() const;

	int Peek(char* dst, int size) const;

	/**
	 * \brief deqSize만큼 버퍼에서 제거한다.
	 * \param deqSize 제거할 크기
	 * \return 삭제한 데이터의 크기
	 */
	int Dequeue(int deqSize);


	/**
	 * \brief
	 * \return front의 주소
	 */
	char* GetFront() const
	{
		return &_buffer[_front];
	}

	/**
	 * \brief
	 * \return rear의 주소
	 */
	char* GetRear() const
	{
		return &_buffer[_rear];
	}

	/**
	 * \brief
	 * \return 내부 버퍼의 시작 위치
	 */
	char* GetBuffer() const
	{
		return _buffer;
	}

	void Clear()
	{
		_front = 0;
		_rear = 0;
	}

	/**
	 * \brief size만큼 front를 이동시킨다.
	 * \param size 이동시킬 front의 크기
	 */
	void MoveFront(const int size)
	{
		_front = (_front + size) % BUFFER_SIZE;
	}

	/**
	 * \brief size만큼 rear를 이동시킨다.
	 * \param size
	 */
	void MoveRear(const int size)
	{
		_rear = (_rear + size) % BUFFER_SIZE;
	}


	/**
	 * \brief 추가로 삽입할 수 있는 크기.
	 * \return
	 */
	 //rear+1 == front일 때 가득 찬 것이므로 1 빼야 함. 
	uint32 FreeSize() const { return BUFFER_SIZE - Size() - 1; }

private:

	const int BUFFER_SIZE;
	const int ALLOC_SIZE;

	char* _buffer = nullptr;

	/**
	 * \brief 데이터가 들어있는 위치
	 */
	int _front;
	/**
	 * \brief 데이터 삽입할 위치. 데이터는 전까지 있음.
	 */
	int _rear;
};