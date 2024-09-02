#pragma once
#include <chrono>
#include "Types.h"

class DBConnection
{
public:
    DBConnection(psh::LPCSTR ip, psh::uint32 port, psh::LPCSTR id, psh::LPCSTR pass, psh::LPCSTR db);

    static void LibInit();

    ~DBConnection();


    std::chrono::milliseconds Query(psh::LPCSTR query, ...);

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
    struct Imple;
    std::unique_ptr<Imple> pImple;
};
