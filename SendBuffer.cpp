#include "stdafx.h"
#include "SendBuffer.h"




SendBuffer SendBuffer::Alloc()
{
	return SendBuffer(*CSendBuffer::Alloc());
}

SendBuffer::~SendBuffer()
{
	buffer.Release();
}