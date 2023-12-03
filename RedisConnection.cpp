#include "stdafx.h"
#include "RedisConnection.h"
#include <atlconv.h>
#include <optional>

bool RedisConnection::Set(const std::string& key, const std::string& value)
{
	auto ret = conn.set(key, value);
	conn.sync_commit();

	return ret.get().ok();
}

bool RedisConnection::Del(const std::string& key)
{
	std::string k(key.begin(), key.end());

	auto ret = conn.del({ "DEL",key });
	conn.sync_commit();

	return ret.get().ok();
}

bool RedisConnection::SetExpired(const std::string& key, const std::string& value, int expireSec)
{

	auto ret = conn.set_advanced(key, value, true, expireSec);
	conn.sync_commit();

	return ret.get().ok();
}

optional<String> RedisConnection::Get(const string& key)
{

	auto result = conn.get(key.c_str());
	conn.sync_commit();

	auto rep = result.get();

	if ( !rep.ok() )
	{
		return{};
	}

	if ( rep.is_null() )
	{
		return{};
	}
	auto cRet = move(rep.as_string());

	String ret(cRet.begin(), cRet.end());

	return ret;
}
