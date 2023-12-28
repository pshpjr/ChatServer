#pragma once
#include "TLSPool.h"
#include "Macro.h"
struct NetHeader;

class CRecvBuffer
{
	USE_TLS_POOL(CRecvBuffer);
	friend class RecvExecutable;
	friend class IOCP;
	friend class Group;
	friend class Session;
public:
	template<typename T>
	CRecvBuffer& operator >> (T& value);

	CRecvBuffer& operator >>(LPWSTR value);
	CRecvBuffer& operator >>(String& value);

	void GetWstr(LPWSTR arr, int size);
	void GetCstr(LPSTR arr, int size);

	static void Decode(char staticKey, NetHeader* head);
	static bool ChecksumValid(NetHeader* head);
//private:
	CRecvBuffer() = default;

	~CRecvBuffer() = default;

	void CanPop(int64 size) const;

	static CRecvBuffer* Alloc(char* front, char* rear)
	{
		const auto ret = _pool.Alloc();
		ret->Clear();
		ret->IncreaseRef(L"AllocInc");
		ret->_front = front;
		ret->_rear = rear;
		return ret;
	}

	long IncreaseRef(LPCWCH cause = L"")
	{
		auto refIncResult = InterlockedIncrement(&_refCount);

#ifdef SERIAL_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { refIncResult,content,0 };
#endif

		return refIncResult;
	}

	void Release(LPCWCH cause = L"")
	{
		auto refResult = InterlockedDecrement(&_refCount);

#ifdef SERIAL_DEBUG
		auto index = InterlockedIncrement(&debugIndex);
		release_D[index % debugSize] = { refResult,content,0 };
#endif

		if ( refResult == 0 )
		{
			_pool.Free(this);
		}
	}

	void Clear();

	char* GetFront() const;
	char* GetRear() const;
	void MoveWritePos(int size);
	void MoveReadPos(int size);
	int	CanPopSize(void) const;

private:
	char* _front = nullptr;
	char* _rear = nullptr;
	long _refCount = 0;

	static TlsPool<CRecvBuffer, 0, false> _pool;

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
inline CRecvBuffer& CRecvBuffer::operator>>(T& value)
{
	static_assert( is_scalar_v<T> );
	CanPop(sizeof(T));

	value = *reinterpret_cast<T*>(_front);
	_front += sizeof(T);

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}