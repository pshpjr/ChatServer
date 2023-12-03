#pragma once
#include "Types.h"
#include "TLSPool.h"
#include "Protocol.h"
#include <stdexcept>
class Player;

//#define SERIAL_DEBUG
class CSerializeBuffer
{
	friend class SessionManager;
	friend class TLSPool<CSerializeBuffer, 0, false>;
	friend class SingleThreadObjectPool<CSerializeBuffer, 0, false>;
	friend class PostSendExecutable;
	friend class IOCP;
	friend class NormalIOCP;
	friend class Session;
	friend class RecvExecutable;

	enum bufferOption { BUFFER_SIZE = 256 };

#pragma pack(1)

	struct LANHeader {
		uint16 len;
	};


	struct NetHeader {
		char code;
		unsigned short len;
		char randomKey;
		unsigned char checkSum;
	};
#pragma pack()



	CSerializeBuffer(const CSerializeBuffer& other) = delete;

	CSerializeBuffer(CSerializeBuffer&& other) noexcept = delete;

	CSerializeBuffer& operator=(const CSerializeBuffer& other) = delete;

	CSerializeBuffer& operator=(CSerializeBuffer&& other) noexcept = delete;

public:

#ifdef SERIAL_DEBUG
	//DEBUG
	struct RelastinReleaseEncrypt_D {
		long refCount;
		LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 2000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

#endif

	/// <summary>
	/// Alloc을 받으면 레퍼런스가 1임. 
	/// </summary>
	/// <returns></returns>
	static CSerializeBuffer* Alloc() 
	{
		auto ret = _pool.Alloc();
		ret->Clear();
		ret->IncreaseRef(L"AllocInc");
		return ret;
	}

	long IncreaseRef(LPCWCH content) { 
		auto refIncResult = InterlockedIncrement(&_refCount);	

#ifdef SERIAL_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { refIncResult,content,0 };
#endif

	return refIncResult; 
	}

	void Release(LPCWCH content)
	{
		auto refResult = InterlockedDecrement(&_refCount);

#ifdef SERIAL_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { refResult,content,0 };
#endif

		if (refResult == 0)
		{
			_pool.Free(this);
		}
	}


	int	GetDataSize() const { return static_cast< int >( _rear - _data ); }
private:
	//버퍼랑 데이터 위치는 고정. 헤더랑 딴 건 가변
	CSerializeBuffer() :_buffer(new char[BUFFER_SIZE + sizeof(NetHeader)]), _head(_buffer), _data(_buffer + sizeof(NetHeader)), _front(_data), _rear(_data)
	{
		InitializeSRWLock(&_encodeLock);

	}

	~CSerializeBuffer()
	{

		delete[] _buffer;
	}

	void Clear()
	{
		_head = _buffer;
		_front = _data;
		_rear = _data;
		isEncrypt = 0;
	}
	void writeNetHeader(int code);


	void Encode(char staticKey);
	void encode(char staticKey);
	void Decode(char staticKey);

	bool checksumValid();


	char* GetDataPtr(void) const { return _data; }
	int getBufferSize() const { return  _bufferSize; }

	ULONG GetFullSize() const { return ULONG(_rear - _head); }



	void MoveWritePos(int size) { _rear += size; }
	void MoveReadPos(int size) { _front += size; }


	int	GetPacketSize(void) const { return static_cast<int>(_rear - _front); }
	char* GetFront() const { return _front; }
	void PacketEnd() { _front = _rear; }

	char* GetHead() const { return _head; }

	void writeLanHeader();

	void setEncryptHeader(NetHeader header);

	void canPush(int size) 
	{
		if ((BUFFER_SIZE - (_rear - _front)) < size)
			throw std::invalid_argument{ "size is bigger than free space in buffer" };
	}
	void canPop(int size) 
	{ 
		if (_rear - _front < size)
			throw std::invalid_argument{ "size is bigger than data in buffer" };
	}

public:
	template<typename T>
	CSerializeBuffer& operator << (const T& value);

	//CSerializeBuffer& operator <<(unsigned char value);
	//CSerializeBuffer& operator <<(char value);

	//CSerializeBuffer& operator <<(unsigned short value);
	//CSerializeBuffer& operator <<(short value);

	//CSerializeBuffer& operator <<(unsigned int value);
	//CSerializeBuffer& operator <<(int value);

	//CSerializeBuffer& operator <<(unsigned long value);
	//CSerializeBuffer& operator <<(long value);

	//CSerializeBuffer& operator <<(unsigned long long value);
	//CSerializeBuffer& operator <<(long long value);

	//CSerializeBuffer& operator <<(float value);
	//CSerializeBuffer& operator <<(double value);

	CSerializeBuffer& operator <<(LPWSTR value);
	CSerializeBuffer& operator <<(LPCWSTR value);
	CSerializeBuffer& operator <<(String& value);

	template<typename T>
	CSerializeBuffer& operator >> (T& value);

	//CSerializeBuffer& operator >>(unsigned char& value);
	//CSerializeBuffer& operator >>(char& value);

	//CSerializeBuffer& operator >>(unsigned short& value);
	//CSerializeBuffer& operator >>(short& value);

	//CSerializeBuffer& operator >>(unsigned int& value);
	//CSerializeBuffer& operator >>(int& value);

	//CSerializeBuffer& operator >>(unsigned long& value);
	//CSerializeBuffer& operator >>(long& value);

	//CSerializeBuffer& operator >>(unsigned long long& value);
	//CSerializeBuffer& operator >>(long long& value);

	//CSerializeBuffer& operator >>(float& value);
	//CSerializeBuffer& operator >>(double& value);

	CSerializeBuffer& operator >>(LPWSTR value);
	CSerializeBuffer& operator >>(String& value);

	void GetWSTR(LPWSTR arr, int size);
	void GetCSTR(LPSTR arr, int size);

	void SetWSTR(LPCWSTR arr, int size);

	void SetCSTR(LPCSTR arr, int size);

private:
	char* _buffer = nullptr;
	char* _data = nullptr;
	char*  _head = nullptr;
	char* _front = nullptr;
	char* _rear = nullptr;
	int _bufferSize = BUFFER_SIZE;
	char isEncrypt = false;

	static TLSPool<CSerializeBuffer, 0, false> _pool;

	public:
	long _refCount = 0;

	SRWLOCK _encodeLock;
};

template<typename T>
inline CSerializeBuffer& CSerializeBuffer::operator<<(const T& value)
{
	static_assert(is_scalar_v<T>);
	canPush(sizeof(T));

	*(T*)(_rear) = value;
	_rear += sizeof(T);

	return *this;
}

template<typename T>
inline CSerializeBuffer& CSerializeBuffer::operator>>(T& value)
{
	static_assert(is_scalar_v<T>);
	canPop(sizeof(T));

	value = *(T*)(_front);
	_front += sizeof(T);

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}
