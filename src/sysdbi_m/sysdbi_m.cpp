/*
* Copyright (c) 2017-2018 the BasicX authors
* All rights reserved.
*
* The project sponsor and lead author is Xu Rendong.
* E-mail: xrd@ustc.edu, QQ: 277195007, WeChat: ustc_xrd
* See the contributors file for names of other contributors.
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

#include <chrono>
#include <iostream>

#include <common/Format/Format.hpp>

#include "sysdbi_m_.h"

namespace basicx {

	SysDBI_M_P::SysDBI_M_P()
		: m_log_cate( "<SYSDBI_M>" ) {
		m_syslog = SysLog_S::GetInstance();
	}

	SysDBI_M_P::~SysDBI_M_P() {
		for( auto it_ci = m_map_mysql_pool.begin(); it_ci != m_map_mysql_pool.end(); it_ci++ ) {
			if( it_ci->second != nullptr ) {
				delete it_ci->second;
				it_ci->second = nullptr;
			}
		}
	}

	void SysDBI_M_P::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show/* = 0*/ ) {
		m_syslog->LogWrite( log_level, log_cate, log_info );
		m_syslog->LogPrint( log_level, log_cate, "LOG>: " + log_info ); // ����̨
	}

	int32_t SysDBI_M_P::AddConnect( size_t connect_number, std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database, std::string charset ) {
		std::string log_info;
		int32_t added_number = 0;
		ConnectInfo* connect_info = nullptr;

		FormatLibrary::StandardLibrary::FormatTo( log_info, "��ʼ��� {0} �� {1} �� MySQL ���� ...", connect_number, host_name );
		LogPrint( syslog_level::c_info, m_log_cate, log_info );
		
		m_mysql_pool_lock.lock();
		auto it_ci = m_map_mysql_pool.find( host_name );
		if( it_ci != m_map_mysql_pool.end() ) { // �Ѵ��ڸ� Host ������
			connect_info = it_ci->second;
		}
		else { // ��û�и� Host ������
			connect_info = new ConnectInfo();
			connect_info->m_host_name = host_name;
			connect_info->m_host_port = host_port;
			connect_info->m_user_name = user_name;
			connect_info->m_user_pass = user_pass;
			connect_info->m_database = database;
			connect_info->m_charset = charset;
			m_map_mysql_pool[host_name] = connect_info;
		}
		m_mysql_pool_lock.unlock();

		for( size_t i = 0; i < connect_number; i++ ) {
			if( MYSQL* connection = CreateConnect( host_name, host_port, user_name, user_pass, database, charset ) ) {
				connect_info->m_idle_connect_list_lock.lock();
				connect_info->m_list_idle_connect.push_back( connection );
				connect_info->m_idle_connect_list_lock.unlock();
				added_number++;
			}
		}
		
		FormatLibrary::StandardLibrary::FormatTo( log_info, "��� MySQL ������ɣ���������� {0}/{1} ����\r\n", added_number, connect_number );
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		return added_number;
	}

	MYSQL* SysDBI_M_P::CreateConnect( std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database, std::string charset ) {
		std::string log_info;
		
		MYSQL* connection = nullptr;
		connection = mysql_init( connection );
		if( nullptr == connection ) {
			log_info = "�����µ� MySQL ���Ӷ���ʧ�ܣ�\r\n";
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
			return nullptr;
		}
		
		bool opt_value = false;
		mysql_options( connection, MYSQL_OPT_RECONNECT, &opt_value ); // �� mysql_real_connect ֮ǰ mysql_init ֮��ʹ�� mysql_options ���ã����Ժ� mysql_query ֮ǰ����ʹ�� mysql_ping �����жϣ���������Ѿ��Ͽ������Զ�����
		
		int32_t connect_times = 1;
		MYSQL* result = mysql_real_connect( connection, host_name.c_str(), user_name.c_str(), user_pass.c_str(), database.c_str(), host_port, nullptr, CLIENT_MULTI_RESULTS );
		
		while( !result && connect_times <= 10 ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "���� {0} �ĵ� {1} ������ʧ�ܣ�", host_name, connect_times );
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );
			LogPrint( syslog_level::c_warn, m_log_cate, GetLastError( connection ) );
			std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
			connect_times++;
			result = mysql_real_connect( connection, host_name.c_str(), user_name.c_str(), user_pass.c_str(), database.c_str(), host_port, nullptr, CLIENT_MULTI_RESULTS );
		}
		
		if( !result ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "���� {0} �� MySQL ����ʧ�ܣ�\r\n", host_name );
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
			if( connection != nullptr ) {
				delete connection;
				connection = nullptr;
			}
		}
		
		return connection;
	}

	MYSQL* SysDBI_M_P::GetConnect( std::string host_name, std::string database/* = ""*/, std::string charset/* = ""*/ ) {
		std::string log_info;

		if( "" == host_name ) {
			return nullptr;
		}

		MYSQL* connection = nullptr;
		ConnectInfo* connect_info = nullptr;

		m_mysql_pool_lock.lock();
		auto it_ci = m_map_mysql_pool.find( host_name );
		if( it_ci != m_map_mysql_pool.end() ) { // �Ѵ��ڸ� Host ������
			connect_info = it_ci->second;
			connect_info->m_idle_connect_list_lock.lock();
			if( !connect_info->m_list_idle_connect.empty() ) {
				connection = connect_info->m_list_idle_connect.back();
				connect_info->m_list_idle_connect.pop_back();
			}
			connect_info->m_idle_connect_list_lock.unlock();
		}
		m_mysql_pool_lock.unlock();

		if( nullptr == connection && nullptr == connect_info ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "������ {0} �� MySQL ���ӣ�\r\n", host_name );
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
		else if( nullptr == connection && connect_info != nullptr ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "û�п��е� {0} �� MySQL ���ӣ����Խ����µ����� ...", host_name );
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );

			connection = CreateConnect( connect_info->m_host_name, connect_info->m_host_port, connect_info->m_user_name, connect_info->m_user_pass, connect_info->m_database, connect_info->m_charset );
		}
		else if( connection != nullptr && connect_info != nullptr && mysql_ping( connection ) != 0 ) { // connection != nullptr �Ļ�Ӧ�ñ��� connect_info != nullptr
			FormatLibrary::StandardLibrary::FormatTo( log_info, "�õ��� {0} �� MySQL �����ѶϿ��������������� ...", host_name );
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );
			try {
				mysql_close( connection );
			}
			catch( ... ) {}
			if( connection != nullptr ) {
				delete connection;
				connection = nullptr;
			}
			connection = CreateConnect( connect_info->m_host_name, connect_info->m_host_port, connect_info->m_user_name, connect_info->m_user_pass, connect_info->m_database, connect_info->m_charset );
		}

		if( connection != nullptr ) {
			if( "" != database ) {
				SelectDB( connection, host_name, database ); // ����Ļ� connection �ᱻ�� nullptr
			}
			if( "" != charset ) {
				CharsetDB( connection, host_name, charset ); // ����Ļ� connection �ᱻ�� nullptr
			}
		}

		return connection;
	}

	bool SysDBI_M_P::SelectDB( MYSQL*& connection, std::string host_name, std::string database ) {
		std::string log_info;

		if( nullptr == connection || "" == database ) {
			return false;
		}

		if( mysql_select_db( connection, database.c_str() ) != 0 ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "ѡ�����ݿ� {0} ʱ��������{1}\r\n", database, mysql_error( connection ) );
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
			try {
				mysql_close( connection );
			}
			catch( ... ) {}
			if( connection != nullptr ) {
				delete connection;
				connection = nullptr; // �� nullptr
			}
			return false;
		}

		return true;
	}

	bool SysDBI_M_P::CharsetDB( MYSQL*& connection, std::string host_name, std::string charset ) {
		std::string log_info;

		if( nullptr == connection || "" == charset ) {
			return false;
		}

		std::string sql_query;
		FormatLibrary::StandardLibrary::FormatTo( sql_query, "SET NAMES {0};", charset );
		std::string error_info;
		if( Query_E( connection, sql_query, error_info ) < 0 ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "�����ַ��� {0} ʱ��������{1}\r\n", charset, error_info );
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
			try {
				mysql_close( connection );
			}
			catch( ... ) {}
			if( connection != nullptr ) {
				delete connection;
				connection = nullptr; // �� nullptr
			}
			return false;
		}

		return true;
	}

	void SysDBI_M_P::ReturnConnect( std::string host_name, MYSQL*& connection ) {
		if( nullptr == connection ) {
			return;
		}

		ConnectInfo* connect_info = nullptr;
		m_mysql_pool_lock.lock();
		auto it_ci = m_map_mysql_pool.find( host_name );
		if( it_ci != m_map_mysql_pool.end() ) { // �Ѵ��ڸ� Host ������
			connect_info = it_ci->second;
			connect_info->m_idle_connect_list_lock.lock();
			size_t idle_number = connect_info->m_list_idle_connect.size();
			connect_info->m_idle_connect_list_lock.unlock();
			if( idle_number >= 16 ) { // ������� 16 ������
				try {
					mysql_close( connection );
				}
				catch( ... ) {}
				if( connection != nullptr ) {
					delete connection;
					connection = nullptr;
				}
			}
			else {
				connect_info->m_idle_connect_list_lock.lock();
				connect_info->m_list_idle_connect.push_back( connection );
				connect_info->m_idle_connect_list_lock.unlock();
			}
		}
		else { // ��û�и� Host ������
			try {
				mysql_close( connection );
			}
			catch( ... ) {}
			if( connection != nullptr ) {
				delete connection;
				connection = nullptr;
			}
		}
		m_mysql_pool_lock.unlock();
	}

	int64_t SysDBI_M_P::Query_E( MYSQL*& connection, std::string& sql_query, std::string& error_info ) {
		if( nullptr == connection ) {
			error_info = "��ѯ��Ҫ�����ݿ�����Ϊ�գ�";
			return -1;
		}
		if( "" == sql_query ) {
			error_info = "��ѯ���Ϊ�գ�";
			return -1;
		}

		try {
			mysql_ping( connection ); // ��� mysql_options �����ӶϿ����Զ�����
			if( mysql_real_query( connection, sql_query.c_str(), (unsigned long)sql_query.length() ) != 0 ) {
				FormatLibrary::StandardLibrary::FormatTo( error_info, "��ѯ����ִ��ʧ�ܣ�{0}", mysql_error( connection ) );
				return -1;
			}
			else {
				if( 0 == mysql_field_count( connection ) ) {
					return (int64_t)mysql_affected_rows( connection );
				}
				else {
					FormatLibrary::StandardLibrary::FormatTo( error_info, "��ѯ����ִ��ʧ�ܣ�{0}", mysql_error( connection ) );
					return -1;
				}
			}
		}
		catch( ... ) {
			error_info = "��ѯ����ִ�з���δ֪����";
			return -1;
		}

		return -1;
	}

	MYSQL_RES* SysDBI_M_P::Query_R( MYSQL*& connection, std::string& sql_query, std::string& error_info ) {
		if( nullptr == connection ) {
			error_info = "��ѯ��Ҫ�����ݿ�����Ϊ�գ�";
			return nullptr;
		}
		if( "" == sql_query ) {
			error_info = "��ѯ���Ϊ�գ�";
			return nullptr;
		}

		try {
			mysql_ping( connection ); // ��� mysql_options �����ӶϿ����Զ�����
			if( mysql_real_query( connection, sql_query.c_str(), (unsigned long)sql_query.length() ) != 0 ) {
				FormatLibrary::StandardLibrary::FormatTo( error_info, "��ѯ������ѯʧ�ܣ�{0}", mysql_error( connection ) );
				return nullptr;
			}
			else {
				return mysql_store_result( connection );
			}
		}
		catch( ... ) {
			error_info = "��ѯ������ѯ����δ֪����";
			return nullptr;
		}

		return nullptr;
	}

	std::string SysDBI_M_P::GetLastError( MYSQL*& connection ) {
		if( nullptr == connection ) {
			return "";
		}

		if( 0 == mysql_errno( connection ) ) {
			return "not have error";
		}

		std::string error_info;
		FormatLibrary::StandardLibrary::FormatTo( error_info, "({0}) {1} [{2}]", mysql_errno( connection ), mysql_error( connection ), (const char*)mysql_get_server_info( connection ) );
		return error_info;
	}

	SysDBI_M* SysDBI_M::m_instance = nullptr;

	SysDBI_M::SysDBI_M()
		: m_sysdbi_m_p( nullptr ) {
		try {
			m_sysdbi_m_p = new SysDBI_M_P();
		}
		catch( ... ) {}
		m_instance = this;
	}

	SysDBI_M::~SysDBI_M() {
		if( m_sysdbi_m_p != nullptr ) {
			delete m_sysdbi_m_p;
			m_sysdbi_m_p = nullptr;
		}
	}

	SysDBI_M* SysDBI_M::GetInstance() {
		return m_instance;
	}

	int32_t SysDBI_M::AddConnect( size_t connect_number, std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database, std::string charset ) {
		return m_sysdbi_m_p->AddConnect( connect_number, host_name, host_port, user_name, user_pass, database, charset );
	}

	MYSQL* SysDBI_M::CreateConnect( std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database, std::string charset ) {
		return m_sysdbi_m_p->CreateConnect( host_name, host_port, user_name, user_pass, database, charset );
	}

	MYSQL* SysDBI_M::GetConnect( std::string host_name, std::string database/* = ""*/, std::string charset/* = ""*/ ) {
		return m_sysdbi_m_p->GetConnect( host_name, database, charset );
	}

	bool SysDBI_M::SelectDB( MYSQL*& connection, std::string host_name, std::string database ) {
		return m_sysdbi_m_p->SelectDB( connection, host_name, database );
	}

	bool SysDBI_M::CharsetDB( MYSQL*& connection, std::string host_name, std::string charset ) {
		return m_sysdbi_m_p->CharsetDB( connection, host_name, charset );
	}

	void SysDBI_M::ReturnConnect( std::string host_name, MYSQL*& connection ) {
		return m_sysdbi_m_p->ReturnConnect( host_name, connection );
	}

	int64_t SysDBI_M::Query_E( MYSQL*& connection, std::string& sql_query, std::string& error_info ) {
		return m_sysdbi_m_p->Query_E( connection, sql_query, error_info );
	}

	MYSQL_RES* SysDBI_M::Query_R( MYSQL*& connection, std::string& sql_query, std::string& error_info ) {
		return m_sysdbi_m_p->Query_R( connection, sql_query, error_info );
	}

	std::string SysDBI_M::GetLastError( MYSQL*& connection ) {
		return m_sysdbi_m_p->GetLastError( connection );
	}

} // namespace basicx
