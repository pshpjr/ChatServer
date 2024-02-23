#include "stdafx.h"
#include "CRecvBuffer.h"
#include <stdexcept>

#include "CRingBuffer.h"
#include "Protocol.h"


constexpr int RECV_INIT_SIZE = 2000;
constexpr int RECV_INIT_MULTIPLIER = 3;


//TlsPool<CRecvBuffer, 0> CRecvBuffer::_pool(RECV_INIT_SIZE, RECV_INIT_MULTIPLIER);

CRecvBuffer& CRecvBuffer::operator>>(String& value)
{
	WORD len = 0;
	operator >> (len);
	CanPop(len);
	len /= sizeof(WCHAR);
	value.resize(len);

	_buffer->Peek(reinterpret_cast<char*>(value.data()),len * sizeof(WCHAR));
	_buffer->Dequeue(len * sizeof(WCHAR));
	
	_size -= len * sizeof(WCHAR);
	return *this;
}

void CRecvBuffer::Clear()
{
	_buffer = nullptr;
	_size = 0;
}


int CRecvBuffer::CanPopSize(void) const
{
	return _size;
}



void CRecvBuffer::GetWstr(const LPWSTR arr, const int strLen)
{
	CanPop(strLen);
	_buffer->Peek(reinterpret_cast<char*>(arr),strLen*sizeof(WCHAR));
	_buffer->Dequeue(strLen*sizeof(WCHAR));
	
	_size -= strLen * sizeof(WCHAR);
}

void CRecvBuffer::GetCstr(const LPSTR arr, const int size)
{
	CanPop(size);
	_buffer->Peek(arr,size);
	_buffer->Dequeue(size);
	_size -= size;
}

void CRecvBuffer::CanPop(const int64 size) const
{
	if ( CanPopSize() < size )
	{
		throw std::invalid_argument { "size is bigger than data in buffer" };
	}
}

