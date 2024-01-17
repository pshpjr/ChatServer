#include "stdafx.h"
#include "CRecvBuffer.h"
#include "Protocol.h"

constexpr int RECV_INIT_SIZE = 2000;
constexpr int RECV_INIT_MULTIPLIER = 3;


TlsPool<CRecvBuffer, 0> CRecvBuffer::_pool(RECV_INIT_SIZE, RECV_INIT_MULTIPLIER);

CRecvBuffer& CRecvBuffer::operator>>(const LPWSTR value)
{
	const size_t strlen = wcslen(( LPWCH ) _front) + 1;
	CanPop(strlen);
	wcscpy_s(value, strlen, ( wchar_t* ) _front);
	_front += strlen * sizeof(WCHAR);
	return *this;
}

CRecvBuffer& CRecvBuffer::operator>>(String& value)
{
	WORD len = 0;
	operator >> (len);
	CanPop(len);
	len /= sizeof(WCHAR);
	value = String(( wchar_t* ) _front, len);

	_front += len * sizeof(WCHAR);
	return *this;
}

void CRecvBuffer::Clear()
{
	_front = nullptr;
	_rear = nullptr;
}

char* CRecvBuffer::GetFront() const
{
	return _front;
}

char* CRecvBuffer::GetRear() const
{
	return _rear;
}

void CRecvBuffer::MoveWritePos(const int size)
{
	_rear += size;
}

void CRecvBuffer::MoveReadPos(const int size)
{
	_front += size;
}

int CRecvBuffer::CanPopSize(void) const
{
	return static_cast< int >( _rear - _front );
}

void CRecvBuffer::Decode(const char staticKey, NetHeader* head)
{
	char* decodeData = ( char* ) head + offsetof(NetHeader, checkSum);
	const unsigned short decodeLen = head->len + static_cast<unsigned short>(sizeof(NetHeader) - offsetof(NetHeader, checkSum));

	char p = 0;
	char e = 0;
	char oldP = 0;
	for ( int i = 1; i <= decodeLen; i++ )
	{
		p = ( *decodeData ) ^ ( e + staticKey + i );
		e = *decodeData;
		*decodeData = p ^ ( oldP + head->randomKey + i );
		oldP = p;
		decodeData++;
	}
}

bool CRecvBuffer::ChecksumValid(NetHeader* head)
{
	const char* checkIndex = ( char* ) head + sizeof(NetHeader);
	//TODO: 아래 방식이 잘못되면 위로 복구.
	const int checkLen = head->len;
	unsigned char payloadChecksum = 0;
	for ( int i = 0; i < checkLen; i++ )
	{
		payloadChecksum += checkIndex[i];
	}
	if ( payloadChecksum != head->checkSum )
	{
		return false;
	}
	return true;

}

void CRecvBuffer::GetWstr(const LPWSTR arr, const int strLen)
{
	CanPop(strLen);
	wcscpy_s(arr, strLen, ( wchar_t* ) _front);
	_front += strLen * sizeof(WCHAR);
}

void CRecvBuffer::GetCstr(const LPSTR arr, const int size)
{
	CanPop(size);
	memcpy_s(arr, size, _front, size);
	_front += size;
}

void CRecvBuffer::CanPop(const int64 size) const
{
	if ( CanPopSize() < size )
	{
		throw std::invalid_argument { "size is bigger than data in buffer" };
	}
}

