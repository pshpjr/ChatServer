#include "CRecvBuffer.h"
#include <stdexcept>
#include "Macro.h"
#include "CRingBuffer.h"


constexpr int RECV_INIT_SIZE = 2000;
constexpr int RECV_INIT_MULTIPLIER = 3;


//TlsPool<CRecvBuffer, 0> CRecvBuffer::_pool(RECV_INIT_SIZE, RECV_INIT_MULTIPLIER);

CRecvBuffer& CRecvBuffer::operator>>(String& value)
{
    psh::uint16 len = 0;
    operator >>(len);

    const unsigned int popBytes = len * sizeof(psh::WCHAR);
    CanPop(popBytes);
    value.resize(len);

    _buffer->Peek(reinterpret_cast<char*>(value.data()), popBytes);
    _buffer->Dequeue(popBytes);

    _size -= popBytes;
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


void CRecvBuffer::GetWstr(const psh::LPWSTR arr, const int strLen)
{
    CanPop(strLen);
    _buffer->Peek(reinterpret_cast<char*>(arr), strLen * sizeof(psh::WCHAR));
    _buffer->Dequeue(strLen * sizeof(psh::WCHAR));

    _size -= strLen * sizeof(psh::WCHAR);
}

void CRecvBuffer::GetCstr(const psh::LPSTR arr, const int size)
{
    CanPop(size);
    _buffer->Peek(arr, size);
    _buffer->Dequeue(size);
    _size -= size;
}

CRecvBuffer::CRecvBuffer(CRingBuffer *buffer, const psh::int32 size): _buffer(buffer)
                                                             , _size(size)
{
}

void CRecvBuffer::CanPop(const psh::uint64 popByte) const
{
    if (CanPopSize() < popByte)
    {
        ASSERT_CRASH(false, "CannotPop");
        throw std::invalid_argument{"size is bigger than data in buffer"};
    }
}
