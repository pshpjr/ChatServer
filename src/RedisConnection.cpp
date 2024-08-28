#include "RedisConnection.h"
#include <optional>
#include "stdafx.h"
//#include <atlbase.h>
#include "CLogger.h"
#include "CoreGlobal.h"

bool RedisConnection::Set(const std::string& key, const std::string& value)
{
    auto ret = _conn.set(key, value);
    _conn.sync_commit();

    return ret.get().ok();
}

bool RedisConnection::Del(const std::string& key)
{
    std::string k(key.begin(), key.end());

    auto ret = _conn.del({"DEL", key});
    _conn.sync_commit();

    return ret.get().ok();
}

bool RedisConnection::SetExpired(const std::string& key, const std::string& value, const int expireSec)
{
    auto ret = _conn.set_advanced(key, value, true, expireSec);
    _conn.sync_commit();

    return ret.get().ok();
}

std::optional<String> RedisConnection::Get(const std::string& key)
{
    auto result = _conn.get(key.c_str());
    _conn.sync_commit();

    const auto rep = result.get();

    if (!rep.ok())
    {
        return {};
    }

    if (rep.is_null())
    {
        String wKey(key.length(), L'\n');
        wKey.assign(key.begin(), key.end());
        gLogger->Write(L"Redis", CLogger::LogLevel::Debug, L"invalid key %s", wKey.c_str());

        return {};
    }
    auto cRet = rep.as_string();

    String ret(cRet.begin(), cRet.end());

    return ret;
}
