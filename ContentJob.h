#pragma once
#include "TLSPool.h"
class CSerializeBuffer;

class ContentJob
{
public:
	enum class ePacketType 
	{
		None,
		Connect,
		Disconnect,
		Packet
	};

	ePacketType _type;
	CSerializeBuffer* _buffer;

	static ContentJob* Alloc() 
	{
		return _pool.Alloc();
	}

	static void Free(ContentJob* job) 
	{
		return _pool.Free(job);
	}

private:
	ContentJob() : _type(ePacketType::None),_buffer(nullptr)
	{

	}


	static TLSPool<ContentJob, 0, false> _pool;
};

