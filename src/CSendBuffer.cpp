#include "CSendBuffer.h"
#include "stdafx.h"


TlsPool<CSendBuffer, 0> CSendBuffer::_pool(SERIAL_INIT_SIZE, SERIAL_INIT_MULTIPLIER);

void CSendBuffer::WriteLanHeader()
{
    *(reinterpret_cast<uint16*>(_front) - 1) = GetDataSize();
    _head = (char*)(reinterpret_cast<uint16*>(_front) - 1);
}

void CSendBuffer::WriteNetHeader(const uint8 code) const
{
    const char* checksumIndex = GetDataPtr();

    NetHeader tmpHeader;
    tmpHeader.len = GetDataSize();
    const int checkLen = tmpHeader.len;
    unsigned char checkSum = 0;

    for (int i = 0; i < checkLen; i++)
    {
        checkSum += checksumIndex[i];
    }

    tmpHeader.checkSum = checkSum;
    tmpHeader.code = code;
    tmpHeader.randomKey = 0x31;
    //head->randomKey = rand()%256;

    *reinterpret_cast<NetHeader*>(GetHead()) = tmpHeader;
}

void CSendBuffer::TryEncode(const char staticKey)
{
    if (!isEncrypt)
    {
        AcquireSRWLockExclusive(&_encodeLock);
        if (!isEncrypt)
        {
            WriteNetHeader(dfPACKET_CODE);

            Encode(staticKey);
        }

        ReleaseSRWLockExclusive(&_encodeLock);
    }
}

CSendBuffer::CSendBuffer()
    : _data(_buffer + sizeof(NetHeader))
    , _head(_buffer)
    , _front(_data)
    , _rear(_data)
{
    InitializeSRWLock(&_encodeLock);
}

bool CSendBuffer::CopyData(CSendBuffer& dst) const
{
    memcpy_s(dst.GetDataPtr(), dst.CanPushSize(), GetFront(), CanPopSize());

    dst.MoveWritePos(CanPopSize());

    return true;
}

void CSendBuffer::Encode(const char staticKey)
{
    using Header = NetHeader;

    const auto head = (Header*)GetHead();
    const unsigned char randomKey = head->randomKey;

    unsigned char* encodeData = &head->checkSum;
    const int encodeLen = head->len + 1;

    char p = 0;
    char e = 0;

    for (short i = 1; i <= encodeLen; i++)
    {
        p = *encodeData ^ p + randomKey + i;
        e = p ^ (e + staticKey + i);
        *encodeData = e;
        encodeData++;
    }

    isEncrypt++;
}


CSendBuffer& CSendBuffer::operator<<(const LPWSTR value)
{
    //insert null terminated string to buffer
    const size_t strlen = wcslen(value) + 1;
    CanPush(strlen);
    wcscpy_s(reinterpret_cast<wchar_t*>(_rear), strlen, value);
    _rear += strlen * sizeof(WCHAR);
    return *this;
}

CSendBuffer& CSendBuffer::operator<<(const LPCWSTR value)
{
    //insert null terminated string to buffer
    const size_t strlen = wcslen(value) + 1;
    SetWstr(value, strlen);
    return *this;
}

CSendBuffer& CSendBuffer::operator<<(const String& value)
{
    operator<<(static_cast<uint16>(value.length()));
    SetWstr(value.c_str(), value.length());

    return *this;
}

void CSendBuffer::SetWstr(const LPCWSTR arr, const int size)
{
    CanPush(size);
    wmemcpy_s((LPWSTR)_rear, size, arr, size);
    _rear += size * sizeof(WCHAR);
}

void CSendBuffer::SetCstr(const LPCSTR arr, const int size)
{
    CanPush(size);
    memcpy_s(_rear, size, arr, size);
    _rear += size;
}
