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
	SessionID _id;
	void Free();

	static ContentJob* Alloc() 
	{
		 auto ret = _pool.Alloc();

		 ret->_buffer = nullptr;
		 ret->_type = ePacketType::None;

		 return ret;
	}
	static ContentJob* Alloc(CSerializeBuffer* buffer);



private:
	ContentJob() : _type(ePacketType::None),_buffer(nullptr)
	{

	}


	static TLSPool<ContentJob, 0, false> _pool;
};

