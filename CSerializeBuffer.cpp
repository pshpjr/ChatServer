#include "stdafx.h"
#include "CSerializeBuffer.h"

#include "Protocol.h"

TLSPool<CSerializeBuffer, 0, false> CSerializeBuffer::_pool;
TLSPool<ContentSerializeBuffer, 0, false> ContentSerializeBuffer::_pool;

void CSerializeBuffer::writeLanHeader()
{
	*(((uint16*)_front)-1) = GetDataSize();
	_head = (char*)(((uint16*)_front) - 1);
}

void CSerializeBuffer::writeNetHeader(int code)
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

void CSerializeBuffer::Encode(char staticKey)
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

long CSerializeBuffer::IncreaseRef(LPCWCH content)
{
	auto refIncResult = InterlockedIncrement(&_refCount);

#ifdef SERIAL_DEBUG
	auto index = InterlockedIncrement(&debugIndex);
	release_D[index % debugSize] = { refIncResult,content,0 };
#endif

	return refIncResult;
}

void CSerializeBuffer::Release(LPCWCH content)
{
	auto refResult = InterlockedDecrement(&_refCount);

#ifdef SERIAL_DEBUG
	auto index = InterlockedIncrement(&debugIndex);
	release_D[index % debugSize] = { refResult,content,0 };
#endif

	_release(refResult);
}

CSerializeBuffer* CSerializeBuffer::Alloc()
{
	auto ret = _pool.Alloc();
	ret->Clear();
	ret->IncreaseRef(L"AllocInc");
	return ret;
}

bool CSerializeBuffer::CopyData(CSerializeBuffer& dst)
{
	memcpy_s(dst.GetHead(), dst.GetFullSize(), GetFront(), GetPacketSize());

	dst.MoveWritePos(GetPacketSize());
}

void CSerializeBuffer::encode(char staticKey)
{
	using Header = CSerializeBuffer::NetHeader;

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

void CSerializeBuffer::Decode(char staticKey, NetHeader* head)
{
	using Header = CSerializeBuffer::NetHeader;

	char* decodeData = (char*)head + offsetof(NetHeader,checkSum);
	int decodeLen = head->len + sizeof(NetHeader) - offsetof(NetHeader, checkSum);

	char p = 0;
	char e = 0;
	char oldP = 0;
	for (int i = 1; i <= decodeLen; i++)
	{
		p = (*decodeData) ^ (e + staticKey + i);
		e = *decodeData;
		*decodeData = p ^ (oldP + head->randomKey + i);
		oldP = p;
		decodeData++;
	}
	isEncrypt--;
}

bool CSerializeBuffer::checksumValid(NetHeader* head)
{
	char* checkIndex = ( char* ) head + sizeof(NetHeader);
	//TODO: 아래 방식이 잘못되면 위로 복구.
	int checkLen = head->len;
	unsigned char payloadChecksum = 0;
	for (int i = 0; i < checkLen; i++)
	{
		payloadChecksum += *checkIndex;
		checkIndex++;
	}
	if (payloadChecksum != head->checkSum)
		return false;
	return true;

}

//
//CSerializeBuffer& CSerializeBuffer::operator<<(unsigned char value)
//{
//	*(unsigned char*)_rear = value;
//	_rear += sizeof(unsigned char);
//
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(char value)
//{
//	*(_rear) = value;
//	_rear += sizeof(char);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(unsigned short value)
//{
//	*(unsigned short*)_rear = value;
//	_rear += sizeof(unsigned short);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(short value)
//{
//	*(short*)(_rear) = value;
//	_rear += sizeof(unsigned short);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(unsigned int value)
//{
//	*(unsigned int*)(_rear) = value;
//	_rear += sizeof(unsigned int);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(int value)
//{
//	*(int*)(_rear) = value;
//	_rear += sizeof(unsigned int);
//
//	return *this;
//}
//
//
//CSerializeBuffer& CSerializeBuffer::operator<<(unsigned long value)
//{
//	*(unsigned long*)(_rear) = value;
//	_rear += sizeof(unsigned long);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(long value)
//{
//	*(long*)(_rear) = value;
//	_rear += sizeof(long);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(unsigned long long value)
//{
//	*(unsigned long long*)(_rear) = value;
//	_rear += sizeof(unsigned long long);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(long long value)
//{
//	*(long long*)(_rear) = value;
//	_rear += sizeof(long long);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(float value)
//{
//	*(float*)(_rear) = value;
//	_rear += sizeof(float);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator<<(double value)
//{
//	*(double*)(_rear) = value;
//	_rear += sizeof(double);
//
//	return *this;
//}

CSerializeBuffer& CSerializeBuffer::operator<<(LPWSTR value)
{

	//insert null terminated string to buffer
	//int strlen = wcslen(value) + 1;
	size_t strlen = wcslen(value)+1;
	canPush(strlen);
	wcscpy_s((wchar_t*)_rear, strlen, value);
	_rear += strlen * sizeof(WCHAR);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(LPCWSTR value)
{
	//insert null terminated string to buffer
	//int strlen = wcslen(value) + 1;
	size_t strlen = wcslen(value)+1;
	canPush(strlen);
	wcscpy_s((wchar_t*)_rear, strlen, value);

	_rear += strlen * sizeof(WCHAR);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(String& value)
{
	canPush(value.size());
	operator<<((uint16)(value.size() * sizeof(WCHAR)));

	value.copy((LPWCH)_rear, value.size());
	_rear += value.size() * sizeof(WCHAR);

	return *this;
}

//
//CSerializeBuffer& CSerializeBuffer::operator>>(unsigned char& value)
//{
//	value = *(unsigned char*)(_front);
//	_front += sizeof(unsigned char);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(char& value)
//{
//	value = *(char*)(_front);
//	_front += sizeof(unsigned char);
//
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(unsigned short& value)
//{
//	value = *(unsigned short*)(_front);
//	_front += sizeof(unsigned short);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(short& value)
//{
//	value = *(short*)(_front);
//	_front += sizeof(short);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(unsigned int& value)
//{
//	value = *(unsigned int*)(_front);
//	_front += sizeof(unsigned int);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(int& value)
//{
//	value = *(int*)(_front);
//	_front += sizeof(int);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(unsigned long& value)
//{
//	value = *(unsigned long*)(_front);
//	_front += sizeof(unsigned long);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(long& value)
//{
//	value = *(long*)(_front);
//	_front += sizeof(long);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(unsigned long long& value)
//{
//	value = *(unsigned long long*)(_front);
//	_front += sizeof(unsigned long long);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(long long& value)
//{
//	value = *(long long*)(_front);
//	_front += sizeof(long long);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(float& value)
//{
//	value = *(float*)(_front);
//	_front += sizeof(float);
//	return *this;
//}
//
//CSerializeBuffer& CSerializeBuffer::operator>>(double& value)
//{
//	value = *(double*)(_front);
//	_front += sizeof(double);
//	return *this;
//}

CSerializeBuffer& CSerializeBuffer::operator>>(String& value)
{
	WORD len = 0;
	operator >> (len);
	canPop(len);
	len /= sizeof(WCHAR);
	value = String((wchar_t*)_front, len);

	_front += len * sizeof(WCHAR);
	return *this;
}

void CSerializeBuffer::GetWSTR(LPWSTR arr, int strLen)
{
	canPop(strLen);
	wcscpy_s(arr, strLen, (wchar_t*)_front);
	_front += strLen * sizeof(WCHAR);
}

void CSerializeBuffer::GetCSTR(LPSTR arr, int size)
{
	canPop(size);
	memcpy_s(arr, size, _front,size);
	_front += size;
}

void CSerializeBuffer::SetWSTR(LPCWSTR arr, int size)
{
	canPush(size);
	wcscpy_s((LPWSTR)_rear, size, (wchar_t*)arr);
	_rear += size * sizeof(WCHAR);
}

void CSerializeBuffer::SetCSTR(LPCSTR arr, int size)
{
	canPop(size);
	memcpy_s(_rear, size, arr, size);
	_rear += size;
}

CSerializeBuffer& CSerializeBuffer::operator>>(LPWSTR value)
{
	size_t strlen = wcslen((LPWCH)_front) + 1;
	canPop(strlen);
	wcscpy_s(value, strlen, (wchar_t*)_front);
	_front += strlen * sizeof(WCHAR);
	return *this;
}

void ContentSerializeBuffer::_release(int refCount)
{
	if ( refCount == 0 )
	{
		_pool.Free(this);
	}
}
