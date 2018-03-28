/*
* Copyright (c) 2017-2018 the BasicX authors
* All rights reserved.
*
* The project sponsor and lead author is Xu Rendong.
* E-mail: xrd@ustc.edu, QQ: 277195007, WeChat: ustc_xrd
* You can get more information at https://xurendong.github.io
* For names of other contributors see the contributors file.
*
* Commercial use of this code in source and binary forms is
* governed by a LGPL v3 license. You may get a copy from the
* root directory. Or else you should get a specific written
* permission from the project author.
*
* Individual and educational use of this code in source and
* binary forms is governed by a 3-clause BSD license. You may
* get a copy from the root directory. Certainly welcome you
* to contribute code of all sorts.
*
* Be sure to retain the above copyright notice and conditions.
*/

#ifndef BASICX_SYSDBI_M_SYSDBI_M_P_H
#define BASICX_SYSDBI_M_SYSDBI_M_P_H

#include <map>
#include <list>
#include <mutex>

#include <syslog/syslog.h>

#include "sysdbi_m.h"

namespace basicx {

#pragma pack( push )
#pragma pack( 1 )

	struct ConnectInfo
	{
		std::string m_host_name;
		int32_t m_host_port;
		std::string m_user_name;
		std::string m_user_pass;
		std::string m_database;
		std::string m_charset;

		std::mutex m_idle_connect_list_lock;
		std::list<MYSQL*> m_list_idle_connect;

		ConnectInfo() 
			: m_host_name( "" )
			, m_host_port( 0 )
			, m_user_name( "" )
			, m_user_pass( "" )
			, m_database( "" )
			, m_charset( "" ) {
		}

		~ConnectInfo() {
			for( auto iter = m_list_idle_connect.begin(); iter != m_list_idle_connect.end(); iter++ ) {
				try {
					mysql_close( *iter );
				}
				catch( ... ) {}
				if( (*iter) != nullptr )
				{
					delete (*iter);
					(*iter) = nullptr;
				}
			}
		}
	};

#pragma pack( pop )

	class SysDBI_M_P
	{
	public:
		SysDBI_M_P();
		~SysDBI_M_P();

	public:
		void LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show = 0 );

	public:
		int32_t AddConnect( size_t connect_number, std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database, std::string charset );
		MYSQL* CreateConnect( std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database, std::string charset );
		MYSQL* GetConnect( std::string host_name, std::string database = "", std::string charset = "" );
		bool SelectDB( MYSQL*& connection, std::string host_name, std::string database );
		bool CharsetDB( MYSQL*& connection, std::string host_name, std::string charset );
		void ReturnConnect( std::string host_name, MYSQL*& connection );
		int64_t Query_E( MYSQL*& connection, std::string& sql_query, std::string& error_info );
		MYSQL_RES* Query_R( MYSQL*& connection, std::string& sql_query, std::string& error_info );
		std::string GetLastError( MYSQL*& connection );

	public:
		std::mutex m_mysql_pool_lock;
		std::map<std::string, ConnectInfo*> m_map_mysql_pool;

	private:
		SysLog_S* m_syslog;
		std::string m_log_cate;
	};

} // namespace basicx

#endif // BASICX_SYSDBI_M_SYSDBI_M_P_H
