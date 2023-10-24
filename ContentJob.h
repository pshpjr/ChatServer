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

	void Free();

	static ContentJob* Alloc() 
	{
		return _pool.Alloc();
	}
	static ContentJob* Alloc(CSerializeBuffer* buffer)
	{
		auto ret =  _pool.Alloc();

		ret->_buffer = buffer;
		buffer->IncreaseRef();
		return ret;
	}


private:
	ContentJob() : _type(ePacketType::None),_buffer(nullptr)
	{

	}


	static TLSPool<ContentJob, 0, false> _pool;
};

