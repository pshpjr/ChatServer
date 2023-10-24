#pragma once
#include "Types.h"

class Player;

class CSerializeBuffer
{
	friend class SessionManager;
	enum bufferOption { BUFFER_SIZE = 3000 };

	struct HeapBreakDebug
	{
		char emptySpace[100];
	};

public:
	CSerializeBuffer() :_head(new char[BUFFER_SIZE]), _buffer(_head), _front(_buffer), _rear(_buffer)
	{

	}
	CSerializeBuffer(char* buffer) : _buffer(buffer), _front(_buffer), _rear(_buffer) {  }


	CSerializeBuffer(const CSerializeBuffer& other)
		: _buffer(other._buffer),
		_front(other._front),
		_rear(other._rear),
		_bufferSize(other._bufferSize)
	{
	}

	CSerializeBuffer(CSerializeBuffer&& other) noexcept
		: _buffer(other._buffer),
		_front(other._front),
		_rear(other._rear),
		_bufferSize(other._bufferSize)
	{
		other._buffer = nullptr;
		other._front = nullptr;
		other._rear = nullptr;
		other._bufferSize = 0;
	}

	CSerializeBuffer& operator=(const CSerializeBuffer& other)
	{
		if (this == &other)
			return *this;
		_buffer = other._buffer;
		_front = other._front;
		_rear = other._rear;
		_bufferSize = other._bufferSize;
		return *this;
	}

	CSerializeBuffer& operator=(CSerializeBuffer&& other) noexcept
	{
		if (this == &other)
			return *this;
		_buffer = other._buffer;
		_front = other._front;
		_rear = other._rear;
		_bufferSize = other._bufferSize;
		return *this;
	}

	~CSerializeBuffer()
	{
		for(int  i = 0; i < 100; ++i)
		{
			if(_heapBreakDebug.emptySpace[i] != 0)
				DebugBreak();
		}

		delete[] _head;
	}

	void Clear()
	{
		_front = _buffer;
		_rear = _buffer;
	}
	void MakeHeader()
	{
		_rear += sizeof(uint16);
		_front = _rear;
	}
	void Seal()
	{
		writeHeader();
		_front = _rear;
	}


	int getBufferSize() const { return  _bufferSize; }
	int		GetPacketSize(void) const { return static_cast<int>(_rear - _front); }
	int		GetFullSize() const { return static_cast<int>(_rear - _buffer); }
	char* GetFullBuffer() const { return _head; }

	char* GetBufferPtr(void) const { return _buffer; }
	void		MoveWritePos(int size) { _rear += size; }
	void		MoveReadPos(int size) { _front += size; }

public:
	void writeHeader();


	CSerializeBuffer& operator <<(unsigned char value);
	CSerializeBuffer& operator <<(char value);

	CSerializeBuffer& operator <<(unsigned short value);
	CSerializeBuffer& operator <<(short value);

	CSerializeBuffer& operator <<(unsigned int value);
	CSerializeBuffer& operator <<(int value);

	CSerializeBuffer& operator <<(unsigned long value);
	CSerializeBuffer& operator <<(long value);

	CSerializeBuffer& operator <<(unsigned long long value);
	CSerializeBuffer& operator <<(long long value);

	CSerializeBuffer& operator <<(float value);
	CSerializeBuffer& operator <<(double value);

	CSerializeBuffer& operator <<(LPWSTR value);
	CSerializeBuffer& operator<<(LPCWSTR value);
	CSerializeBuffer& operator <<(String& value);

	CSerializeBuffer& operator >>(unsigned char& value);
	CSerializeBuffer& operator >>(char& value);

	CSerializeBuffer& operator >>(unsigned short& value);
	CSerializeBuffer& operator >>(short& value);

	CSerializeBuffer& operator >>(unsigned int& value);
	CSerializeBuffer& operator >>(int& value);

	CSerializeBuffer& operator >>(unsigned long& value);
	CSerializeBuffer& operator >>(long& value);

	CSerializeBuffer& operator >>(unsigned long long& value);
	CSerializeBuffer& operator >>(long long& value);

	CSerializeBuffer& operator >>(float& value);
	CSerializeBuffer& operator >>(double& value);

	CSerializeBuffer& operator >>(LPWSTR value);
	CSerializeBuffer& operator >>(String& value);
private:
	char* _head = nullptr;
	char* _buffer = nullptr;
	char* _front = nullptr;
	char* _rear = nullptr;
	int _bufferSize = BUFFER_SIZE;
	HeapBreakDebug _heapBreakDebug = {0,};

};
