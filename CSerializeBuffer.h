#pragma once
#include "Types.h"
#include "TLSPool.h"
#include "Protocol.h"
#include <stdexcept>
class Player;

//#define SERIAL_DEBUG
class CSerializeBuffer
{
	USE_TLS_POOL(CSerializeBuffer);

	friend class SessionManager;
	friend class PostSendExecutable;
	friend class IOCP;
	friend class NormalIOCP;
	friend class Session;
	friend class RecvExecutable;

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


	/// <summary>
	/// Alloc을 받으면 레퍼런스가 1임. 
	/// </summary>
	/// <returns></returns>
	static CSerializeBuffer* Alloc();

	long IncreaseRef(LPCWCH content);

	void Release(LPCWCH content);

	int	GetDataSize() const { return static_cast< int >( _rear - _data ); }

	bool CopyData(CSerializeBuffer& dst);

public:
	template<typename T>
	CSerializeBuffer& operator << (const T& value);

	CSerializeBuffer& operator <<(LPWSTR value);
	CSerializeBuffer& operator <<(LPCWSTR value);
	CSerializeBuffer& operator <<(String& value);

	template<typename T>
	CSerializeBuffer& operator >> (T& value);

	CSerializeBuffer& operator >>(LPWSTR value);
	CSerializeBuffer& operator >>(String& value);

	void GetWSTR(LPWSTR arr, int size);
	void GetCSTR(LPSTR arr, int size);

	void SetWSTR(LPCWSTR arr, int size);

	void SetCSTR(LPCSTR arr, int size);



protected:
	virtual void _release(int refCount)
	{
		if ( refCount == 0 )
		{
			_pool.Free(this);
		}
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

	void Decode(char staticKey, NetHeader* head);
	bool checksumValid(NetHeader* head);

	//recv시 헤더부터 여기에 다 받음. 
	//send시 여기에 데이터가 담김. 
	char* GetDataPtr(void) const { return _data; }


	int getBufferSize() const { return  _bufferSize; }
	int canPushSize() const {
		return (int)(BUFFER_SIZE - distance(_data, _rear) ); }
	ULONG SendDataSize() const { return ULONG(_rear - _head); }

	void MoveWritePos(int size) { _rear += size; }
	void MoveReadPos(int size) { _front += size; }

	int	CanPopSize(void) const { return static_cast<int>(_rear - _front); }
	char* GetFront() const { return _front; }
	char* GetRear() const { return _rear; }

	char* GetHead() const { return _head; }

	void writeLanHeader();


	void canPush(int64 size) 
	{
		if ( canPushSize() < size)
			throw std::invalid_argument{ "size is bigger than free space in buffer" };
	}
	void canPop(int64 size)
	{ 
		if (CanPopSize() < size)
			throw std::invalid_argument{ "size is bigger than data in buffer" };
	}

protected:
	CSerializeBuffer(char* front, char* rear):_buffer(nullptr),_head(nullptr),_data(nullptr),_front(front),_rear(rear)
	{

	}
	virtual ~CSerializeBuffer()
	{

		delete[] _buffer;
	}

protected:
	char* _front = nullptr;
	char* _rear = nullptr;

private:
	//버퍼랑 데이터 위치는 고정. 헤더랑 딴 건 가변
	CSerializeBuffer() :_buffer(new char[BUFFER_SIZE + sizeof(NetHeader)]), _head(_buffer), _data(_buffer + sizeof(NetHeader)), _front(_data), _rear(_data)
	{
		InitializeSRWLock(&_encodeLock);

	}

private:
	enum bufferOption { BUFFER_SIZE = 4096 };


	char* _buffer = nullptr;
	char* _data = nullptr;
	char*  _head = nullptr;
	int _bufferSize = BUFFER_SIZE;
	char isEncrypt = false;

	static TLSPool<CSerializeBuffer, 0, false> _pool;

public:
	long _refCount = 0;

	SRWLOCK _encodeLock;

#ifdef SERIAL_DEBUG
	//DEBUG
	struct RelastinReleaseEncrypt_D
	{
		long refCount;
		LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 1000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

#endif
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


class ContentSerializeBuffer : public CSerializeBuffer
{
	USE_TLS_POOL(ContentSerializeBuffer);

public:
	static ContentSerializeBuffer* Alloc(char* front, char* rear) 
	{
		auto ret = _pool.Alloc();
		ret->Clear();
		ret->IncreaseRef(L"AllocInc");
		ret->_front = front;
		ret->_rear = rear;
		return ret;
	}
private:
	ContentSerializeBuffer() :CSerializeBuffer(nullptr, nullptr)
	{

	}

	~ContentSerializeBuffer() {}
private:
	static TLSPool<ContentSerializeBuffer, 0, false> _pool;
protected:
	void _release(int refCount) override;
};

