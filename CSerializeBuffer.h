#pragma once
#include "Types.h"
#include "TLSPool.h"
class Player;


class CSerializeBuffer
{
	friend class SessionManager;
	friend class TLSPool<CSerializeBuffer, 0, false>;
	friend class SingleThreadObjectPool<CSerializeBuffer, 0, false>;
	friend class PostSendExecutable;
	friend class IOCP;
	friend class Session;
	friend class RecvExecutable;
	enum bufferOption { BUFFER_SIZE = 4096 };

#pragma pack(1)

	struct LANHeader {
		uint16 len;
	};


	struct NetHeader {
		char code;
		short len;
		char randomKey;
		unsigned char checkSum;
	};
#pragma pack()

	struct HeapBreakDebug
	{
		char emptySpace[100];
	};

	//버퍼랑 데이터 위치는 고정. 헤더랑 딴 건 가변
	CSerializeBuffer() :_buffer(new char[BUFFER_SIZE+sizeof(NetHeader)]),_head(_buffer), _data(_buffer+sizeof(NetHeader)), _front(_data), _rear(_data)
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

		delete[] _buffer;
	}

	int	GetPacketSize(void) const { return static_cast<int>(_rear - _front); }
	char* GetFront() const { return _front; }
	void PacketEnd() { _front = _rear; }

	template <typename T>
	T& GetContentHeader() 
	{
		return (T*)_front;
	}

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
	void Release()
	{
		if (InterlockedDecrement(&_refCount) == 0)
		{
			_pool.Free(this);
		}
	}
	void IncreaseRef() { InterlockedIncrement(&_refCount); }


	void print() 
	{
		for (unsigned char* begin = (unsigned char*)_head; begin != (unsigned char*)_rear; begin++) {
			printf("%c", *begin);
		}
		printf("\n");
		printf("\n");
		return;
	}
	void writeNetHeader(int code);


	void Encode(char staticKey);
	void Decode(char staticKey);

	bool checksumValid()
	{
		unsigned char payloadChecksum = 0;
		NetHeader* header = (NetHeader*)GetHead();
		char* checkIndex = GetDataPtr();
		//int checkLen = header->len - sizeof(NetHeader);

		//TODO: 아래 방식이 잘못되면 위로 복구.
		int checkLen = GetDataSize();
		for (int i = 0; i < checkLen; i++)
		{
			payloadChecksum += *checkIndex;
			checkIndex++;
		}
		return payloadChecksum == header->checkSum;

	}
private:

	void Clear()
	{
		_head = _buffer;
		_front = _data;
		_rear = _data;
	}






	int getBufferSize() const { return  _bufferSize; }

	int	GetDataSize() const { return static_cast<int>(_rear - _data); }
	char* GetHead() const { return _head; }
	int GetFullSize() const { return _rear - _head; }

	char* GetDataPtr(void) const { return _data; }


	void MoveWritePos(int size) { _rear += size; }
	void MoveReadPos(int size) { _front += size; }
	//makeHeader나 seal 같은 건 컨텐츠에서 할 게 아님. 
	//컨텐츠는 컨텐츠 헤더를 써야 한다. 

	void writeLanHeader();

	void setEncryptHeader(NetHeader header);

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

	void GetSTR(LPWSTR arr, int size);

private:
	char* _buffer = nullptr;
	char* _data = nullptr;
	char*  _head = nullptr;
	char* _front = nullptr;
	char* _rear = nullptr;
	int _bufferSize = BUFFER_SIZE;
	bool isEncrypt = true;
	
	HeapBreakDebug _heapBreakDebug = {0,};

	static TLSPool<CSerializeBuffer, 0, false> _pool;

	long _refCount = 0;
};


