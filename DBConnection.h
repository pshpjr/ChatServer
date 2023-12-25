#pragma once
#pragma comment(lib,"libmysql.lib")
#include "externalHeader/mysql.h"
#include "DBException.h"

class DBConnection
{
public:
	DBConnection(LPCSTR ip, uint32 port, LPCSTR id, LPCSTR pass, LPCSTR db )
	{
		mysql_init(&conn);

		//SetReconnect(true);

		connection = mysql_real_connect(&conn, ip, id, pass, db, port, ( char* ) NULL, 0);
		const char* err = nullptr;
		if ( connection == NULL )
		{
			err = mysql_error(&conn);
			auto num = mysql_errno(&conn);
			throw DBErr(err,num, chrono::milliseconds::zero());

			printf("%d \n", num);
		}
		SetReconnect(true);
	}

	~DBConnection()
	{
		Close();
	}


	std::chrono::milliseconds Query(LPCSTR query, ...)
	{
		using std::chrono::system_clock;
		using std::chrono::duration_cast;
		using std::chrono::duration;
		using std::chrono::milliseconds;
		using namespace std::chrono_literals;
		va_list args;

		va_start(args, query);
		size_t len = vsnprintf(NULL, 0, query, args);
		va_end(args);

		vector<char> vec(len + 1);

		va_start(args, query);
		vsnprintf(&vec[0], len + 1, query, args);
		va_end(args);

		auto start = system_clock::now();
		const char* err;
		
		int query_stat = mysql_query(connection, vec.data());
		if ( query_stat != 0 )
		{
			err = mysql_error(&conn);

			auto num = mysql_errno(&conn);

			switch ( num )
			{
				case CR_SERVER_GONE_ERROR:
				case CR_SERVER_LOST:
				case CR_CONN_HOST_ERROR:
				case CR_SERVER_HANDSHAKE_ERR:
					break;

				default:
				{
					auto dur = duration_cast< milliseconds >( system_clock::now() - start );
					string errStr = to_string(dur.count()) + err;
					throw DBErr(errStr.c_str(),num,dur);
				}
			}
		}
		sql_result = mysql_store_result(connection);


		return duration_cast< milliseconds >( system_clock::now() - start );
	}

	bool next()
	{
		sql_row = mysql_fetch_row(sql_result);
		return sql_row != nullptr;
	}

	char* getString(int index)
	{
		return sql_row[index];
	}



	int getInt(int index)
	{
		return stoi(sql_row[index]);
	}

	double getDouble(int index)
	{
		return stod(sql_row[index]);
	}

	void reset()
	{
		mysql_free_result(sql_result);
	}

	void Close()
	{
		mysql_close(connection);
	}

	void SetReconnect(bool useReconnect)
	{
		bool reconnect = useReconnect;
		mysql_options(&conn, MYSQL_OPT_RECONNECT, &reconnect);
	}

private:
	MYSQL conn;
	MYSQL* connection = NULL;
	MYSQL_RES* sql_result = {};
	MYSQL_ROW sql_row = {};
};

