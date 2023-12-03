#pragma once
#include "Types.h"
#include <cpp_redis/cpp_redis.h>


#pragma comment(lib,"cpp_redis.lib")
#pragma comment(lib,"tacopie.lib")
#include <optional>

class RedisConnection
{
public:
	RedisConnection(LPCSTR ip, uint32 port, uint32 timeout = 0, int32 maxReconnect = 0, uint32 reconnectInterval = 0)
	{
		conn.connect(ip, port, nullptr, timeout, maxReconnect, reconnectInterval);
	}
	~RedisConnection()
	{
		conn.disconnect(false);
	}

	bool SetExpired(const std::string& key, const std::string& value, int expireSec);

	bool Set(const std::string& key, const std::string& value);

	bool Del(const std::string& key);

	optional<String> Get(const std::string& key);
	
private:
	cpp_redis::client conn;
	cpp_redis::connect_state status;
};

