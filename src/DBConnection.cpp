#include "DBConnection.h"
#include "DBException.h"
#include "Macro.h"
#include "externalHeader/mysql.h"
#pragma comment(lib,"libmysql.lib")

struct DBConnection::Imple
{
    MYSQL conn{};
    MYSQL* connection = nullptr;
    MYSQL_RES* sql_result{};
    MYSQL_ROW sql_row{};
};

    DBConnection::DBConnection(const LPCSTR ip, const uint32 port, const LPCSTR id, const LPCSTR pass, const LPCSTR db)
{
    mysql_init(&pImple->conn);

    //SetReconnect(true);

    pImple->connection = mysql_real_connect(&pImple->conn, ip, id, pass, db, port, nullptr, 0);
    if (pImple->connection == nullptr)
    {
        const char* err = nullptr;
        err = mysql_error(&pImple->conn);
        const auto num = mysql_errno(&pImple->conn);
        throw DBErr(err, num, std::chrono::milliseconds::zero());

        printf("%d \n", num);
    }
    SetReconnect(true);
}

void DBConnection::LibInit()
{
    mysql_library_init(0, nullptr, nullptr);
}

DBConnection::~DBConnection()
{
    Close();
}

std::chrono::milliseconds DBConnection::Query(LPCSTR query, ...)
{
    using std::string;
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;
    using namespace std::chrono_literals;

    std::string queryString;

    va_list args;

    va_start(args, query);
    const size_t len = vsnprintf(nullptr, 0, query, args);

    queryString.resize(len + 1);
    vsnprintf(&queryString[0], len + 1, query, args);

    queryString.resize(len);
    va_end(args);

    const auto start = steady_clock::now();

    if (const int query_stat = mysql_real_query(pImple->connection, queryString.c_str()
                                              , static_cast<unsigned long>(queryString.length()));
        query_stat != 0)
    {
        const char *err = mysql_error(&pImple->conn);

        switch (const auto num = mysql_errno(&pImple->conn))
        {
            case CR_SERVER_GONE_ERROR:
            case CR_SERVER_LOST:
            case CR_CONN_HOST_ERROR:
            case CR_SERVER_HANDSHAKE_ERR:
                break;

            default:
            {
                const auto dur = duration_cast<milliseconds>(steady_clock::now() - start);
                const string errStr = std::to_string(dur.count()) + err;
                throw DBErr(errStr.c_str(), num, dur);
            }
        }
    }

    pImple->sql_result = mysql_store_result(pImple->connection);


    return duration_cast<milliseconds>(steady_clock::now() - start);
}

bool DBConnection::next()
{
    pImple->sql_row = mysql_fetch_row(pImple->sql_result);
    return pImple->sql_row != nullptr;
}

char * DBConnection::getString(const int index) const
{
    return pImple->sql_row[index];
}

char DBConnection::getChar(const int index) const
{
    auto value = getInt(index);
    ASSERT_CRASH(-128 <= value && value <= 127, L"NotCharRange");
    return static_cast<char>(value);
}

int DBConnection::getInt(const int index) const
{
    return std::stoi(pImple->sql_row[index]);
}

float DBConnection::getFloat(const int index) const
{
    return std::stof(pImple->sql_row[index]);
}

double DBConnection::getDouble(const int index) const
{
    return std::stod(pImple->sql_row[index]);
}

void DBConnection::reset()
{
    mysql_free_result(pImple->sql_result);
}

void DBConnection::Close()
{
    mysql_close(pImple->connection);
}

void DBConnection::SetReconnect(const bool useReconnect)
{
    const bool reconnect = useReconnect;
    mysql_options(&pImple->conn, MYSQL_OPT_RECONNECT, &reconnect);
}
