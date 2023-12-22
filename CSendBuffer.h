#pragma once
#include "TLSPool.h"
//#define SEND_DEBUG

class Player;
struct NetHeader;
#pragma pack(1)


class CSendBuffer
{
	USE_TLS_POOL(CSendBuffer);

	friend class PostSendExecutable;
	friend class IOCP;
	friend class NormalIOCP;
	friend class Session;

public:

#pragma pack()
	static int64 GetPoolAllockedSize() { return _pool.AllockedCount(); }
	/// <summary>
	/// Alloc을 받으면 레퍼런스가 1임. 
	/// </summary>
	/// <returns></returns>

	static CSendBuffer* Alloc()
	{
		auto ret = _pool.Alloc();
		ret->Clear();
		ret->IncreaseRef(L"AllocInc");
		return ret;
	}

	long IncreaseRef(LPCWCH content = L"")
	{
		auto refIncResult = InterlockedIncrement(&_refCount);

#ifdef SEND_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { refIncResult,content,0 };
#endif

		return refIncResult;
	}

	void Release(LPCWCH content = L"")
	{
		auto refResult = InterlockedDecrement(&_refCount);

#ifdef SEND_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { refResult,content,0 };
#endif

		if ( refResult == 0 )
		{
			_pool.Free(this);
		}
	}

	inline int	GetDataSize() const { return static_cast< int >( _rear - _data ); }

	template<typename T>
	CSendBuffer& operator << (const T& value);
	CSendBuffer& operator <<(LPWSTR value);
	CSendBuffer& operator <<(LPCWSTR value);
	CSendBuffer& operator <<(String& value);

	void SetWSTR(LPCWSTR arr, int size);
	void SetCSTR(LPCSTR arr, int size);

private:
	//버퍼랑 데이터 위치는 고정. 헤더랑 딴 건 가변
	CSendBuffer();
	virtual ~CSendBuffer()
	{

		delete[] _buffer;
	}
	CSendBuffer(const CSendBuffer& other) = delete;

	CSendBuffer(CSendBuffer&& other) noexcept = delete;

	CSendBuffer& operator=(const CSendBuffer& other) = delete;

	CSendBuffer& operator=(CSendBuffer&& other) noexcept = delete;

private:

	bool CopyData(CSendBuffer& dst);

	void Clear()
	{
		_head = _buffer;
		_front = _data;
		_rear = _data;
		isEncrypt = 0;
	}

	inline ULONG SendDataSize() const { return ULONG(_rear - _head); }
	inline char* GetFront() const { return _front; }
	inline char* GetRear() const { return _rear; }
	inline char* GetHead() const { return _head; }
	inline char* GetDataPtr(void) const { return _data; }
	inline void MoveWritePos(int size) { _rear += size; }
	inline int canPushSize() const
	{
		return ( int ) ( BUFFER_SIZE - distance(_data, _rear) );
	}


	inline int CanPopSize() const
	{
		return ( int ) ( distance(_front, _rear) );
	}

	inline void canPush(int64 size)
	{
		if ( canPushSize() < size )
			throw std::invalid_argument { "size is bigger than free space in buffer" };
	}

	//Encrypt
	void writeLanHeader();
	void writeNetHeader(int code);
	void Encode(char staticKey);
	void encode(char staticKey);
	


private:
	enum bufferOption { BUFFER_SIZE = 1024 };

	char* _front = nullptr;
	char* _rear = nullptr;
	char* _buffer = nullptr;
	char* _data = nullptr;
	char*  _head = nullptr;
	int _bufferSize = BUFFER_SIZE;
	char isEncrypt = false;

	static TLSPool<CSendBuffer, 0, false> _pool;

	long _refCount = 0;

	SRWLOCK _encodeLock;


	//DEBUG
#ifdef SEND_DEBUG

	struct RelastinReleaseEncrypt_D
	{
		long refCount;
		LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 1000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

	static void PoolDebug()
	{
		for ( auto node : _pool.allocked )
		{
			if ( node->_data._refCount == 1 )
				DebugBreak();
			int a = 0;
		}
	}

#endif
};

template<typename T>
inline CSendBuffer& CSendBuffer::operator<<(const T& value)
{
	static_assert(is_scalar_v<T>);
	canPush(sizeof(T));

	*(T*)(_rear) = value;
	_rear += sizeof(T);

	return *this;
}



