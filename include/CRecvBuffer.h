#pragma once
#include "CRingBuffer.h"

class CRecvBuffer
{
    //USE_TLS_POOL(CRecvBuffer);
    friend class RecvExecutable;
    friend class IOCP;
    friend class Group;

public:
    template <typename T>
    CRecvBuffer& operator >>(T& value);

    CRecvBuffer& operator >>(String& value);

    void GetWstr(const psh::LPWSTR arr, int size);
    void GetCstr(psh::LPSTR arr, int size);


    //private:
    CRecvBuffer() = default;

    CRecvBuffer(CRingBuffer* buffer, const psh::int32 size);

    ~CRecvBuffer() = default;

    void CanPop(const psh::uint64 popByte) const;

    //	static CRecvBuffer* Alloc(CRingBuffer* buffer, psh::int32 size)
    //	{
    //		const auto ret = _pool.Alloc();
    //		ret->Clear();
    //		ret->IncreaseRef(L"AllocInc");
    //		ret->_buffer = buffer;
    //		ret->_size = size;
    //		return ret;
    //	}
    //
    //	long IncreaseRef(LPCWCH cause = L"")
    //	{
    //		auto refIncResult = InterlockedIncrement(&_refCount);
    //
    //#ifdef SERIAL_DEBUG
    //		auto index = InterlockedIncrement(&debugIndex);
    //		release_D[index % debugSize] = { refIncResult,content,0 };
    //#endif
    //
    //		return refIncResult;
    //	}
    //
    //	void Release(LPCWCH cause = L"")
    //	{
    //		auto refResult = InterlockedDecrement(&_refCount);
    //
    //#ifdef SERIAL_DEBUG
    //		auto index = InterlockedIncrement(&debugIndex);
    //		release_D[index % debugSize] = { refResult,content,0 };
    //#endif
    //
    //		if ( refResult == 0 )
    //		{
    //			_pool.Free(this);
    //		}
    //	}


    [[nodiscard]] int CanPopSize() const;

private:
    void Clear();
    CRingBuffer* _buffer = nullptr;
    psh::int32 _size = 0;
    long _refCount = 0;

    //static TlsPool<CRecvBuffer, 0, false> _pool;

#ifdef SERIAL_DEBUG
	//DEBUG
	struct RelastinReleaseEncrypt_D
	{
		long refCount;
		psh::LPCWSTR location;
		long long contentType;
	};
	static const int debugSize = 1000;

	long debugIndex = 0;
	RelastinReleaseEncrypt_D release_D[debugSize];

#endif
};

template <typename T>
CRecvBuffer& CRecvBuffer::operator>>(T& value)
{
    static_assert(std::is_scalar_v<T>);
    CanPop(sizeof(T));

    _buffer->Peek(reinterpret_cast<char*>(&value), sizeof(T));
    _buffer->Dequeue(sizeof(T));
    _size -= sizeof(T);

    return *this;
}
