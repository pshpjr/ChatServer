#pragma once
#include "TLSPool.h"
class CSerializeBuffer;

class ContentJob
{
	USE_TLS_POOL(ContentJob)
public:
	enum class ePacketType 
	{
		None,
		Connect,
		Disconnect,
		Packet,
		TimeoutCheck
	};

	ePacketType _type;
	CSerializeBuffer* _buffer;

	void Free();

	static ContentJob* Alloc() 
	{
		return _pool.Alloc();
	}
	static ContentJob* Alloc(CSerializeBuffer* buffer);



private:
	ContentJob() : _type(ePacketType::None),_buffer(nullptr)
	{

	}


	static TLSPool<ContentJob, 0, false> _pool;
};

