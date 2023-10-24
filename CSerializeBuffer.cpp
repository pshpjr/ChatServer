#include "stdafx.h"
#include "CSerializeBuffer.h"



void CSerializeBuffer::writeHeader()
{
	*(((uint16*)_front)-1) = GetPacketSize();
}

CSerializeBuffer& CSerializeBuffer::operator<<(unsigned char value)
{
	*(unsigned char*)_rear = value;
	_rear += sizeof(unsigned char);


	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(char value)
{
	*(_rear) = value;
	_rear += sizeof(char);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(unsigned short value)
{
	*(unsigned short*)_rear = value;
	_rear += sizeof(unsigned short);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(short value)
{
	*(short*)(_rear) = value;
	_rear += sizeof(unsigned short);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(unsigned int value)
{
	*(unsigned int*)(_rear) = value;
	_rear += sizeof(unsigned int);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(int value)
{
	*(int*)(_rear) = value;
	_rear += sizeof(unsigned int);

	return *this;
}


CSerializeBuffer& CSerializeBuffer::operator<<(unsigned long value)
{
	*(unsigned long*)(_rear) = value;
	_rear += sizeof(unsigned long);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(long value)
{
	*(long*)(_rear) = value;
	_rear += sizeof(long);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(unsigned long long value)
{
	*(unsigned long long*)(_rear) = value;
	_rear += sizeof(unsigned long long);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(long long value)
{
	*(long long*)(_rear) = value;
	_rear += sizeof(long long);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(float value)
{
	*(float*)(_rear) = value;
	_rear += sizeof(float);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(double value)
{
	*(double*)(_rear) = value;
	_rear += sizeof(double);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(LPWSTR value)
{
	//insert null terminated string to buffer
	//int strlen = wcslen(value) + 1;
	int strlen = 15;

	wcscpy_s((wchar_t*)_rear, strlen, value);
	_rear += strlen * sizeof(WCHAR);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(LPCWSTR value)
{
	//insert null terminated string to buffer
	//int strlen = wcslen(value) + 1;
	int strlen = 15;
	wcscpy_s((wchar_t*)_rear, strlen, value);
	_rear += strlen * sizeof(WCHAR);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(String& value)
{
	operator<<((uint16)(value.size() * sizeof(WCHAR)));

	value.copy((LPWCH)_rear, value.size());
	_rear += value.size() * sizeof(WCHAR);

	return *this;
}


CSerializeBuffer& CSerializeBuffer::operator>>(unsigned char& value)
{
	value = *(unsigned char*)(_front);
	_front += sizeof(unsigned char);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(char& value)
{
	value = *(char*)(_front);
	_front += sizeof(unsigned char);

	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(unsigned short& value)
{
	value = *(unsigned short*)(_front);
	_front += sizeof(unsigned short);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(short& value)
{
	value = *(short*)(_front);
	_front += sizeof(short);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(unsigned int& value)
{
	value = *(unsigned int*)(_front);
	_front += sizeof(unsigned int);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(int& value)
{
	value = *(int*)(_front);
	_front += sizeof(int);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(unsigned long& value)
{
	value = *(unsigned long*)(_front);
	_front += sizeof(unsigned long);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(long& value)
{
	value = *(long*)(_front);
	_front += sizeof(long);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(unsigned long long& value)
{
	value = *(unsigned long long*)(_front);
	_front += sizeof(unsigned long long);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(long long& value)
{
	value = *(long long*)(_front);
	_front += sizeof(long long);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(float& value)
{
	value = *(float*)(_front);
	_front += sizeof(float);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(double& value)
{
	value = *(double*)(_front);
	_front += sizeof(double);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(String& value)
{
	WORD len;
	operator >> (len);

	len /= sizeof(WCHAR);
	value = String((wchar_t*)_front, len);

	_front += len * sizeof(WCHAR);
	return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(LPWSTR value)
{
	int strlen = wcslen((LPWCH)_front) + 1;

	wcscpy_s(value, strlen, (wchar_t*)_front);
	_front += strlen * sizeof(WCHAR);
	return *this;
}
