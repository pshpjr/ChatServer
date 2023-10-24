#pragma once

class CSerializeBuffer;

class ContentJob
{
public:
	enum class ePacketType 
	{
		Connect,
		Disconnect,
		Packet
	};

	ePacketType _type;
	CSerializeBuffer* _buffer;
};

