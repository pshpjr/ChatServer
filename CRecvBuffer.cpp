#include "stdafx.h"
#include "CRecvBuffer.h"
#include "Protocol.h"

int RECV_INIT_SIZE = 2000;
int RECV_INIT_MULTIPLIER = 3;


TLSPool<CRecvBuffer, 0, false> CRecvBuffer::_pool(RECV_INIT_SIZE, RECV_INIT_MULTIPLIER);


CRecvBuffer::CRecvBuffer()
{

}

CRecvBuffer::~CRecvBuffer()
{

}


CRecvBuffer& CRecvBuffer::operator>>(LPWSTR value)
{
	size_t strlen = wcslen(( LPWCH ) _front) + 1;
	canPop(strlen);
	wcscpy_s(value, strlen, ( wchar_t* ) _front);
	_front += strlen * sizeof(WCHAR);
	return *this;
}

CRecvBuffer& CRecvBuffer::operator>>(String& value)
{
	WORD len = 0;
	operator >> (len);
	canPop(len);
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

void CRecvBuffer::MoveWritePos(int size)
{
	_rear += size;
}

void CRecvBuffer::MoveReadPos(int size)
{
	_front += size;
}

int CRecvBuffer::CanPopSize(void) const
{
	return static_cast< int >( _rear - _front );
}

void CRecvBuffer::Decode(char staticKey, NetHeader* head)
{
	char* decodeData = ( char* ) head + offsetof(NetHeader, checkSum);
	unsigned short decodeLen = head->len + (unsigned short)(sizeof(NetHeader) - offsetof(NetHeader, checkSum));

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

bool CRecvBuffer::checksumValid(NetHeader* head)
{
	char* checkIndex = ( char* ) head + sizeof(NetHeader);
	//TODO: 아래 방식이 잘못되면 위로 복구.
	int checkLen = head->len;
	unsigned char payloadChecksum = 0;
	for ( int i = 0; i < checkLen; i++ )
	{
		payloadChecksum += checkIndex[i];
	}
	if ( payloadChecksum != head->checkSum )
		return false;
	return true;

}

void CRecvBuffer::GetWSTR(LPWSTR arr, int strLen)
{
	canPop(strLen);
	wcscpy_s(arr, strLen, ( wchar_t* ) _front);
	_front += strLen * sizeof(WCHAR);
}

void CRecvBuffer::GetCSTR(LPSTR arr, int size)
{
	canPop(size);
	memcpy_s(arr, size, _front, size);
	_front += size;
}

void CRecvBuffer::canPop(int64 size)
{
	if ( CanPopSize() < size )
		throw std::invalid_argument { "size is bigger than data in buffer" };
}

