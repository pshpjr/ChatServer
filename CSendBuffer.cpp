#include "stdafx.h"
#include "Profiler.h"
#include "CSendBuffer.h"

#include "Protocol.h"

int SERIAL_INIT_SIZE = 1000;
int SERIAL_INIT_MULTIPLIER = 3;

TLSPool<CSendBuffer, 0, false> CSendBuffer::_pool(SERIAL_INIT_SIZE, SERIAL_INIT_MULTIPLIER);

void CSendBuffer::writeLanHeader()
{
	*(((uint16*)_front)-1) = GetDataSize();
	_head = (char*)(((uint16*)_front) - 1);
}

void CSendBuffer::writeNetHeader(int code)
{
	NetHeader* head = (NetHeader*)GetHead();
	head->len = GetDataSize();
	head->checkSum = 0;

	char* checksumIndex = GetDataPtr();
	int checkLen = GetDataSize();
	for (int i = 0; i < checkLen; i++)
	{
		head->checkSum += *checksumIndex;
		checksumIndex++;
	}
	head->code = code;

	//head->randomKey = rand()%256;
	head->randomKey = 0x31;
}

void CSendBuffer::Encode(char staticKey)
{
	if (!isEncrypt)
	{
		AcquireSRWLockExclusive(&_encodeLock);
		if(!isEncrypt)
		{
			writeNetHeader(dfPACKET_CODE);

			encode(staticKey);
		}

		ReleaseSRWLockExclusive(&_encodeLock);
	}
}

CSendBuffer::CSendBuffer() :_buffer(new char[BUFFER_SIZE + sizeof(NetHeader)]), _head(_buffer), _data(_buffer + sizeof(NetHeader)), _front(_data), _rear(_data)
{
	InitializeSRWLock(&_encodeLock);
}

bool CSendBuffer::CopyData(CSendBuffer& dst)
{
	memcpy_s(dst.GetDataPtr(), dst.canPushSize(), GetFront(), CanPopSize());

	dst.MoveWritePos(CanPopSize());

	return true;
}

void CSendBuffer::encode(char staticKey)
{
	using Header = NetHeader;

	Header* head = (Header*)GetHead();
	

	char* encodeData = GetDataPtr() - 1;
	int encodeLen = GetDataSize() + 1;

	char p = 0;
	char e = 0;

	for (int i = 1; i <= encodeLen; i++)
	{
		p = (*encodeData) ^ (p + head->randomKey + i);
		e = p ^ (e + staticKey + i);
		*encodeData = e;
		encodeData++;
	}
	isEncrypt++;
}


CSendBuffer& CSendBuffer::operator<<(LPWSTR value)
{

	//insert null terminated string to buffer
	size_t strlen = wcslen(value)+1;
	canPush(strlen);
	wcscpy_s((wchar_t*)_rear, strlen, value);
	_rear += strlen * sizeof(WCHAR);
	return *this;
}

CSendBuffer& CSendBuffer::operator<<(LPCWSTR value)
{
	//insert null terminated string to buffer
	size_t strlen = wcslen(value)+1;
	canPush(strlen);
	wcscpy_s((wchar_t*)_rear, strlen, value);

	_rear += strlen * sizeof(WCHAR);
	return *this;
}

CSendBuffer& CSendBuffer::operator<<(String& value)
{
	canPush(value.size());
	operator<<((uint16)(value.size() * sizeof(WCHAR)));

	value.copy((LPWCH)_rear, value.size());
	_rear += value.size() * sizeof(WCHAR);

	return *this;
}

void CSendBuffer::SetWSTR(LPCWSTR arr, int size)
{
	canPush(size);
	wcscpy_s(( LPWSTR ) _rear, size, ( wchar_t* ) arr);
	_rear += size * sizeof(WCHAR);
}

void CSendBuffer::SetCSTR(LPCSTR arr, int size)
{
	canPush(size);
	memcpy_s(_rear, size, arr, size);
	_rear += size;
}

