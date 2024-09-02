﻿#pragma once
#include <stdexcept>
#include "TLSPool.h"
#include "Protocol.h"

//#define SEND_DEBUG

struct NetHeader;

class CSendBuffer final
{
    USE_TLS_POOL(CSendBuffer)

    friend class IOCP;
    friend class NormalIOCP;
    friend class Session;

public:
     psh::int64 GetPoolAllocatedSize()
    {
        return _pool.AllocatedCount();
    }

    /// <summary>
    /// Alloc을 받으면 레퍼런스가 1임. 
    /// </summary>
    /// <returns></returns>

    static CSendBuffer* Alloc()
    {
        const auto ret = _pool.Alloc();
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
#else
        UNREFERENCED_PARAMETER(content);
#endif

        return refIncResult;
    }

    void Release(LPCWCH content = L"")
    {
        auto refResult = InterlockedDecrement(&_refCount);

#ifdef SEND_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { refResult,content,0 };
#else
        UNREFERENCED_PARAMETER(content);
#endif

        if (refResult == 0)
        {
            _pool.Free(this);
        }
    }

    psh::uint16 GetDataSize() const
    {
        return static_cast<psh::uint16>(_rear - _data);
    }

    template <typename T>
    CSendBuffer& operator <<(const T& value);
    CSendBuffer& operator <<(LPWSTR value);
    CSendBuffer& operator <<(psh::LPCWSTR value);
    CSendBuffer& operator <<(const String& value);

    void SetWstr(psh::LPCWSTR arr, int size);
    void SetCstr(LPCSTR arr, int size);

    int CanPushSize() const
    {
        return static_cast<int>(BUFFER_SIZE - std::distance(_data, _rear));
    }

    //TODO:Private로 이동
    long _refCount = 0;

private:
    //버퍼랑 데이터 위치는 고정. 헤더랑 딴 건 가변
    CSendBuffer();
    ~CSendBuffer() = default;

    CSendBuffer(const CSendBuffer& other) = delete;

    CSendBuffer(CSendBuffer&& other) noexcept = delete;

    CSendBuffer& operator=(const CSendBuffer& other) = delete;

    CSendBuffer& operator=(CSendBuffer&& other) noexcept = delete;

private:
    bool CopyData(CSendBuffer& dst) const;

    void Clear()
    {
        _head = _buffer;
        _front = _data;
        _rear = _data;
        isEncrypt = 0;
    }

    ULONG SendDataSize() const
    {
        return static_cast<ULONG>(_rear - _head);
    }

    char* GetFront() const
    {
        return _front;
    }

    char* GetRear() const
    {
        return _rear;
    }

    char* GetHead() const
    {
        return _head;
    }

    char* GetDataPtr(void) const
    {
        return _data;
    }

    void MoveWritePos(const int size)
    {
        _rear += size;
    }


    int CanPopSize() const
    {
        return static_cast<int>(std::distance(_front, _rear));
    }

    void CanPush(const psh::int64 byteSize) const
    {
        if (CanPushSize() < byteSize)
        {
            throw std::invalid_argument{"size is bigger than free space in buffer"};
        }
    }

    //Encrypt
    void WriteLanHeader();
    void WriteNetHeader(psh::uint8 code) const;
    void TryEncode(char staticKey);
    void Encode(char staticKey);

private:
    enum bufferOption { BUFFER_SIZE = 2048 };


    char _buffer[BUFFER_SIZE + sizeof(NetHeader)];
    char* _data = nullptr;
    char* _head = nullptr;
    char* _front = nullptr;
    char* _rear = nullptr;
    int _bufferSize = BUFFER_SIZE;
    char isEncrypt = false;

    static TlsPool<CSendBuffer, 0, false> _pool;


    SRWLOCK _encodeLock;

    static constexpr int SERIAL_INIT_SIZE = 500;
    static constexpr int SERIAL_INIT_MULTIPLIER = 20;

    //DEBUG
#ifdef SEND_DEBUG

	struct RelastinReleaseEncrypt_D
	{
		long refCount;
		psh::LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 1000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

	//static void PoolDebug()
	//{
	//	for ( auto node : _pool.allocated )
	//	{
	//		if ( node->_data._refCount == 1 )
	//			__debugbreak();
	//		int a = 0;
	//	}
	//}

#endif
};

template <typename T>
CSendBuffer& CSendBuffer::operator<<(const T& value)
{
    static_assert(std::is_scalar_v<T>);
    CanPush(sizeof(T));

    *reinterpret_cast<T*>(_rear) = value;
    _rear += sizeof(T);

    return *this;
}
