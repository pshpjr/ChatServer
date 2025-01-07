#pragma once
#include <chrono>
#include "Types.h"

class DBConnection
{
public:
    DBConnection(psh::LPCSTR ip, psh::uint32 port, psh::LPCSTR id, psh::LPCSTR pass, psh::LPCSTR db);

    static void LibInit();

    ~DBConnection();

    template<typename... Args>
    std::chrono::microseconds QueryFormat(psh::LPCSTR query, Args&&... args);

    [[deprecated("Use QueryFormat(psh::LPCSTR, ...)")]]
    std::chrono::microseconds Query(psh::LPCSTR query, ...);

    //저장 프로시저 호출용
    template<typename... Args>
    std::chrono::microseconds CallFormat(psh::LPCSTR func, Args&&... args );

    bool next();

    [[nodiscard]] char* getString(int index) const;

    [[nodiscard]] char getChar(int index) const;

    [[nodiscard]] int getInt(int index) const;

    [[nodiscard]] float getFloat(int index) const;

    [[nodiscard]] double getDouble(int index) const;

    void reset();

    void Close();

    void SetReconnect(bool useReconnect);

private:
    std::chrono::microseconds ExecuteQuery(const std::string& queryString);

    struct Imple;
    std::unique_ptr<Imple> pImple;
};

template <typename ... Args>
std::chrono::microseconds DBConnection::QueryFormat(psh::LPCSTR query, Args&&... args)
{
    std::string queryString = std::format(query, std::forward<Args>(args)...);

    return ExecuteQuery(queryString);
}

template <typename ... Args>
std::chrono::microseconds DBConnection::CallFormat(psh::LPCSTR func, Args&&... args)
{
    throw std::runtime_error("DBConnection::CallFormat: Not Implemented");
}
