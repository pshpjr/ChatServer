#pragma once
#include <utility>

#include "CSendBuffer.h"

class SendBuffer
{
public:
	SendBuffer(CSendBuffer& other) :buffer(&other) {}
	~SendBuffer();

	SendBuffer(SendBuffer&& other) noexcept
		: buffer{other.buffer}
	{
		other.buffer = nullptr;
	}
	

	SendBuffer(const SendBuffer& other) = delete;
	SendBuffer& operator= (const SendBuffer& other) = delete;

	static SendBuffer Alloc();

	int Size() const
	{ 	
		return buffer->GetDataSize(); 
	}

	CSendBuffer* getBuffer() const
	{
		return buffer;
	}

	template<typename T>
	SendBuffer& operator << (const T& value)
	{
		*buffer << value;
		return *this;
	}

	void SetWstr(LPCWSTR arr, int size) { buffer->SetWstr(arr, size); }
	void SetCstr(LPCSTR arr, int size) { buffer->SetCstr(arr, size); }

	int CanPushSize(){return buffer->CanPushSize();}
private:
	CSendBuffer* buffer;
};