#pragma once
#include "Types.h"
#include "TLSPool.h"
class Player;

class CSerializeBuffer
{
	friend class SessionManager;
	friend class TLSPool<CSerializeBuffer, 0, false>;
	friend class SingleThreadObjectPool<CSerializeBuffer, 0, false>;
	enum bufferOption { BUFFER_SIZE = 4096 };

	struct HeapBreakDebug
	{
		char emptySpace[100];
	};

	CSerializeBuffer() :_head(new char[BUFFER_SIZE]), _buffer(_head), _front(_buffer), _rear(_buffer)
	{

	}

	CSerializeBuffer(const CSerializeBuffer& other) = delete;

	CSerializeBuffer(CSerializeBuffer&& other) noexcept = delete;

	CSerializeBuffer& operator=(const CSerializeBuffer& other) = delete;

	CSerializeBuffer& operator=(CSerializeBuffer&& other) noexcept = delete;

public:

	~CSerializeBuffer()
	{
		DebugBreak();
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

	void IncreaseRef() { InterlockedIncrement(&_refCount); }
	void Release()
	{
		if(InterlockedDecrement(&_refCount) == 0)
		{
			_pool.Free(this);
		}
	}


	int getBufferSize() const { return  _bufferSize; }
	int	GetPacketSize(void) const { return static_cast<int>(_rear - _front); }
	int	GetFullSize() const { return static_cast<int>(_rear - _buffer); }
	char* GetFullBuffer() const { return _head; }

	char* GetBufferPtr(void) const { return _buffer; }
	void MoveWritePos(int size) { _rear += size; }
	void MoveReadPos(int size) { _front += size; }

	void writeHeader();


	/// <summary>
	/// Alloc을 받으면 레퍼런스가 1임. 
	/// </summary>
	/// <returns></returns>
	static CSerializeBuffer* Alloc() 
	{
		auto ret = _pool.Alloc();
		ret->Clear();
		ret->IncreaseRef();
		return ret;
	}


public:
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
	CSerializeBuffer& operator <<(LPCWSTR value);
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
	long _refCount = 0;
	
	HeapBreakDebug _heapBreakDebug = {0,};

	static TLSPool<CSerializeBuffer, 0, false> _pool;
};


