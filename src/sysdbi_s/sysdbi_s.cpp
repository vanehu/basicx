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

#include <common/sysdef.h>
#include <common/Format/Format.hpp>

#ifdef __OS_WINDOWS__
#include <windows.h>
#endif

#include "sysdbi_s_.h"

namespace basicx {

	SysDBI_S_P::SysDBI_S_P()
		: m_log_cate( "<SYSDBI_S>" ) {
		m_syslog = SysLog_S::GetInstance();
	}

	SysDBI_S_P::~SysDBI_S_P() {
	}

	int32_t SysDBI_S_P::Connect( Connection_P_R connection, Recordset_P_R recordset, std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database ) {
		std::string log_info;
		std::string connection_info;
		int32_t result = S_OK;

		::CoInitialize( NULL );

		result = ::CoCreateInstance( ADODB::CLSID_Connection, NULL, CLSCTX_ALL, ADODB::IID__Connection, (void**)&connection );
		if( FAILED( result ) ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "创建数据库连接实例 {0} 失败！\r\n", result );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return result;
		}
		else {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "创建数据库连接实例 {0} 成功。", result );
			m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}

		connection->put_CursorLocation( ADODB::adUseClient ); // 不加这个，可能使查询结果的记录集计数为 -1 且导出记录集结果出错

		FormatLibrary::StandardLibrary::FormatTo( connection_info, "Provider = SQLOLEDB.1; Persist Security Info = False; User ID = {0}; Password = {1}; Initial Catalog = {2}; Data Source = {3}, {4}", user_name, user_pass, database, host_name, host_port );
		try {
			result = connection->Open( _bstr_t( connection_info.c_str() ), "", "", ADODB::adConnectUnspecified );
			if( FAILED( result ) ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "打开数据库连接实例 {0} 失败！\r\n", result );
				m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
				return result;
			}
			else {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "打开数据库连接实例 {0} 成功。", result );
				m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
			}
		}
		catch( _com_error error ) {
			std::wstring w_error_info( (LPCTSTR)error.Description() );
			int32_t number_error_info = WideCharToMultiByte( CP_OEMCP, NULL, w_error_info.c_str(), -1, NULL, 0, NULL, FALSE );
			char* temp_error_info = new char[number_error_info];
			WideCharToMultiByte( CP_OEMCP, NULL, w_error_info.c_str(), -1, temp_error_info, number_error_info, NULL, FALSE );
			std::string error_info = std::string( temp_error_info );
			delete[] temp_error_info;
			FormatLibrary::StandardLibrary::FormatTo( log_info, "打开数据库连接实例 {0} 时发生错误: {1}！\r\n", result, error_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return result = E_FAIL;
		}
		catch( ... ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "打开数据库连接实例 {0} 时发生未知错误！\r\n", result );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return result = E_FAIL;
		}

		connection->put_CommandTimeout( 1000 );

		result = ::CoCreateInstance( ADODB::CLSID_Recordset, NULL, CLSCTX_ALL, ADODB::IID__Recordset, (void**)&recordset );
		if( FAILED( result ) ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "创建记录集实例 {0} 失败！\r\n", result );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return result;
		}
		else {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "创建记录集实例 {0} 成功。\r\n", result );
			m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}

		return result;
	}

	bool SysDBI_S_P::Query( Connection_P_R connection, Recordset_P_R recordset, std::string query ) {
		std::string log_info;

		try {
			recordset->Open( _bstr_t( query.c_str() ), _variant_t( (IDispatch*)connection, true ), ADODB::adOpenKeyset, ADODB::adLockOptimistic, ADODB::adCmdText );
		}
		catch( _com_error error ) {
			std::wstring w_error_info( (LPCTSTR)error.Description() );
			int32_t number_error_info = WideCharToMultiByte( CP_OEMCP, NULL, w_error_info.c_str(), -1, NULL, 0, NULL, FALSE );
			char* temp_error_info = new char[number_error_info];
			WideCharToMultiByte( CP_OEMCP, NULL, w_error_info.c_str(), -1, temp_error_info, number_error_info, NULL, FALSE );
			std::string error_info = std::string( temp_error_info );
			delete[] temp_error_info;
			FormatLibrary::StandardLibrary::FormatTo( log_info, "数据信息查询失败! 错误: {0}！语句：{1}", error_info, query );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}
		catch( ... ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "数据信息查询失败! 错误未知！语句：{0}", query );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		return true;
	}

	long SysDBI_S_P::GetCount( Recordset_P_R recordset ) {
		long row_number = 0;
		recordset->get_RecordCount( &row_number );
		return row_number;
	}

	bool SysDBI_S_P::GetEOF( Recordset_P_R recordset ) {
		if( recordset->GetadoEOF() ) {
			return true;
		}
		return false;
	}

	bool SysDBI_S_P::MoveNext( Recordset_P_R recordset ) {
		std::string log_info;

		if( recordset->GetadoEOF() ) {
			log_info = "已到达记录集的最后一条记录! ";
			m_syslog->LogWrite( syslog_level::c_warn, m_log_cate, log_info );
			return false;
		}

		try {
			recordset->MoveNext();
		}
		catch( ... ) {
			log_info = "移动至下一条记录时发生未知错误! ";
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		return true;
	}

	void SysDBI_S_P::Close( Recordset_P_R recordset ) {
		recordset->Close();
	}

	void SysDBI_S_P::Release( Connection_P_R connection, Recordset_P_R recordset ) {
		if( nullptr != connection ) {
			connection->Release();
			connection = nullptr;
		}
		if( nullptr != recordset ) {
			recordset->Release();
			recordset = nullptr;
		}
		::CoUninitialize();
	}

	SysDBI_S* SysDBI_S::m_instance = nullptr;

	SysDBI_S::SysDBI_S()
		: m_sysdbi_s_p( nullptr ) {
		try {
			m_sysdbi_s_p = new SysDBI_S_P();
		}
		catch( ... ) {}
		m_instance = this;
	}

	SysDBI_S::~SysDBI_S() {
		if( m_sysdbi_s_p != nullptr ) {
			delete m_sysdbi_s_p;
			m_sysdbi_s_p = nullptr;
		}
	}

	SysDBI_S* SysDBI_S::GetInstance() {
		return m_instance;
	}

	int32_t SysDBI_S::Connect( Connection_P_R connection, Recordset_P_R recordset, std::string host_name, int32_t host_port, std::string user_name, std::string user_pass, std::string database ) {
		return m_sysdbi_s_p->Connect( connection, recordset, host_name, host_port, user_name, user_pass, database );
	}

	bool SysDBI_S::Query( Connection_P_R connection, Recordset_P_R recordset, std::string query ) {
		return m_sysdbi_s_p->Query( connection, recordset, query );
	}

	long SysDBI_S::GetCount( Recordset_P_R recordset ) {
		return m_sysdbi_s_p->GetCount( recordset );
	}

	bool SysDBI_S::GetEOF( Recordset_P_R recordset ) {
		return m_sysdbi_s_p->GetEOF( recordset );
	}

	bool SysDBI_S::MoveNext( Recordset_P_R recordset ) {
		return m_sysdbi_s_p->MoveNext( recordset );
	}

	void SysDBI_S::Close( Recordset_P_R recordset ) {
		m_sysdbi_s_p->Close( recordset );
	}

	void SysDBI_S::Release( Connection_P_R connection, Recordset_P_R recordset ) {
		m_sysdbi_s_p->Release( connection, recordset );
	}

} // namespace basicx
