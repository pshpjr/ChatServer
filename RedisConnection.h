#pragma once
#include "Types.h"
#include <cpp_redis/cpp_redis.h>


#pragma comment(lib,"cpp_redis.lib")
#pragma comment(lib,"tacopie.lib")
#include <optional>

class RedisConnection
{
public:
	RedisConnection(const LPCSTR ip, const uint32 port, const uint32 timeout = 0, const int32 maxReconnect = 0, const uint32 reconnectInterval = 0): _status()
	{
		_conn.connect(ip, port, nullptr, timeout, maxReconnect, reconnectInterval);
	}

	~RedisConnection()
	{
		_conn.disconnect(false);
	}

	bool SetExpired(const std::string& key, const std::string& value, int expireSec);

	bool Set(const std::string& key, const std::string& value);

	bool Del(const std::string& key);

	optional<String> Get(const std::string& key);
	
private:
	cpp_redis::client _conn;
	cpp_redis::connect_state _status;
};

