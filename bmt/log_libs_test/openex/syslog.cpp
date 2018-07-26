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
 * Be sure to retain the above copyright notices and conditions.
 */

#include <chrono>
#include <cassert>
#include <fstream>
#include <iostream>

#include <windows.h>

#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Format/Format.hpp"
#include "easylogging++.h"

#include "syslog.h"

INITIALIZE_EASYLOGGINGPP

openex::SysLog* openex::SysLog::m_instance = nullptr;

namespace openex {

	SysLog::SysLog()
	: m_start_time( "" )
	, m_log_folder( "" )
	, m_app_name( "def_app" ) // �ڵ��� SetSysLog ǰĬ�ϣ�����ģ�鹹�캯���е��� LogWrite �� ClearScreen ����ʱ��
	, m_app_version( "def_ver" ) // �ڵ��� SetSysLog ǰĬ�ϣ�����ģ�鹹�캯���е��� LogWrite �� ClearScreen ����ʱ��
	, m_log_n( nullptr ) {
		m_instance = this;
	}

	SysLog::~SysLog() {
		spdlog::drop_all();
		if( m_log_n != nullptr ) {
			delete m_log_n;
			m_log_n = nullptr;
		}
	}

	SysLog* SysLog::GetInstance() {
		return m_instance;
	}

	void SysLog::SetColor( unsigned short color ) {
		HANDLE std_handle = GetStdHandle( STD_OUTPUT_HANDLE ); // ��û��������
		SetConsoleTextAttribute( std_handle, color ); // �����ı���������ɫ
		//CloseHandle( std_handle );
	}

	void write_header( boost::log::sinks::text_file_backend::stream_type& file )
	{
		file << "---------- start ----------\n";
	}

	void write_footer( boost::log::sinks::text_file_backend::stream_type& file )
	{
		file << "---------- End ----------\n";
	}

	void SysLog::InitSysLog( std::string app_name, std::string app_version ) {
		m_app_name = app_name;
		m_app_version = app_version;

		char start_time_buf[64] = { '0' };
		m_now_time_t = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
		localtime_s( &m_now_time_tm, &m_now_time_t );
		strftime( start_time_buf, 64, "%Y-%m-%d %a %H:%M:%S", &m_now_time_tm );
		m_start_time = start_time_buf;

		std::string module_folder = boost::filesystem::initial_path<boost::filesystem::path>().string();
		size_t slash_index = module_folder.rfind( '\\' );
		m_log_folder = module_folder.substr( 0, slash_index ) + "\\log";
		// ��������³�������ʱ��־�ļ�����ɾ�����ģ��������ｨ���Ժ��ټ����ļ����Ƿ����
		if( !boost::filesystem::exists( m_log_folder ) ) {
			boost::filesystem::create_directories( m_log_folder );
		}

		////////////////////////////////////////////////////////////////////////////////////////////////
		
		std::string log_file_path = m_log_folder + "\\" + m_app_name + "_%Y-%m-%d_%3N.log"; // Test_%Y-%m-%d_%H-%M-%S_%5N.log
		boost::shared_ptr<text_sink> sink_n( new text_sink(
			boost::log::keywords::file_name = log_file_path, 
			boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point( 0, 0, 0 ) ) ); // ÿ���½� // ���ܵ� - 2
		m_sink_n = sink_n;
		
		text_sink::locked_backend_ptr backend_n = sink_n->locked_backend();
		backend_n->auto_flush( true ); // true:��������־��false:������־ // ���ܵ� - 3
		//backend_n->set_open_mode( std::ios::app ); // ׷��ģʽ // ���ܵ� - 4
		
		//��Ϊ synchronous_sink �£�׷��ģʽ����Ӱ�����ԣ��ʿ���ÿ�λ����ļ�����Ӱ��
		backend_n->set_file_collector( boost::log::sinks::file::make_collector( boost::log::keywords::target = m_log_folder ) ); // ����Ļ� scan_for_files() �����
		backend_n->scan_for_files(); // Ҳ�������ļ�������뼶ʱ����������־�ļ�����
		
		sink_n->set_formatter( // �����ģ���� warning �� node ���� boost/log/support/date_time.hpp �� decompose_time_of_day ��������� (uint32_t) ǿ��ת��
			boost::log::expressions::format( "%1% %2%" ) // "%1% %2% %3%"
		//	% boost::log::expressions::attr<uint32_t>( "RecordID" )
			% boost::log::expressions::format_date_time<boost::posix_time::ptime>( "TimeStamp", "%Y-%m-%d %H:%M:%S.%f" )
			% boost::log::expressions::smessage );
		
		//boost::log::attributes::counter<uint32_t> record_id( 1, 1 );
		//boost::log::core::get()->add_global_attribute( "RecordID", record_id );
		boost::log::attributes::local_clock timestamp;
		boost::log::core::get()->add_global_attribute( "TimeStamp", timestamp );
		
		sink_n->set_filter( boost::log::expressions::attr<severity_level_n>( "Severity" ) >= debug_n );
		
		//sink_n->locked_backend()->set_open_handler( boost::lambda::_1 << "---------- start ----------\n" );
		//sink_n->locked_backend()->set_close_handler( boost::lambda::_1 << "---------- End ----------\n" );
		
		boost::log::core::get()->add_sink( sink_n );
		
		if( m_log_n ) {
			delete m_log_n;
			m_log_n = nullptr;
		}
		
		m_log_n = new boost::log::sources::severity_logger<severity_level_n>;
		
		////////////////////////////////////////////////////////////////////////////////////////////////

		//el::Configurations log_cfg( "/log_cfg.ini" );
		//el::Loggers::reconfigureAllLoggers( log_cfg );

		el::Configurations default_log_cfg;
		default_log_cfg.setToDefault();
		default_log_cfg.set( el::Level::Info, el::ConfigurationType::Format, "%datetime %level %msg" );
		default_log_cfg.set( el::Level::Info, el::ConfigurationType::ToStandardOutput, "false" );
		//default_log_cfg.set( el::Level::Info, el::ConfigurationType::LogFlushThreshold, "250" );
		el::Loggers::reconfigureLogger( "default", default_log_cfg );
		el::Loggers::addFlag( el::LoggingFlag::ImmediateFlush );

		////////////////////////////////////////////////////////////////////////////////////////////////

		//m_logger_sy = spdlog::daily_logger_st( "logger_sy", "daily.log", 0, 0 );
		m_logger_sy = spdlog::daily_logger_mt( "logger_sy", "daily.log", 0, 0 );
		m_logger_sy->flush_on( spdlog::level::trace );
		m_logger_sy->set_level( spdlog::level::debug );
		spdlog::set_pattern( "%Y-%m-%d %H:%M:%S.%F %z thread:%t %v" );

		//m_sink_as_st = std::make_shared<spdlog::sinks::daily_file_sink_st>( "daily_as.log", 0, 0 );
		//m_sink_as_mt = std::make_shared<spdlog::sinks::daily_file_sink_mt>( "daily_as.log", 0, 0 );
		//m_logger_as = std::make_shared<spdlog::async_logger>( "logger_as", m_sink_as_st, 8192 ); // �� 10240 �����ϻ����
		//m_logger_as = std::make_shared<spdlog::async_logger>( "logger_as", m_sink_as_mt, 8192 ); // �� 10240 �����ϻ����
		//m_logger_as->flush_on( spdlog::level::trace );
	}

	void SysLog::ClearScreen( short row, short col, bool is_head/* = false*/, int32_t wait/* = 0*/ ) {
		Sleep( wait );

		//system( "cls" ); // Ҳ���Լ򵥵����������

		HANDLE std_handle = GetStdHandle( STD_OUTPUT_HANDLE );
		CONSOLE_SCREEN_BUFFER_INFO buffer_info; // ������Ϣ
		GetConsoleScreenBufferInfo( std_handle, &buffer_info );
		COORD position = { col, row };
		SetConsoleCursorPosition( std_handle, position ); // ���ù��λ��
		DWORD chars_written; // �� Windows 10 �±���ʹ�ã��������� NULL ���棬��������
		FillConsoleOutputCharacter( std_handle, ' ', buffer_info.dwSize.X * buffer_info.dwSize.Y, position, &chars_written );

		if( true == is_head ) {
			wchar_t computer_name[64];
			wchar_t account_name[64];
			DWORD size_computer_name = 64;
			DWORD size_account_name = 64;
			GetComputerName( computer_name, &size_computer_name );
			GetUserName( account_name, &size_account_name ); // ��Ҫ Advapi32.lib
			std::string a_log_info;
			std::wstring w_log_info;
			FormatLibrary::StandardLibrary::FormatTo( a_log_info, "\nWelcome to {0}", m_app_name );
			LogPrint( "norm", 1, "<SYSTEM_START>", a_log_info );
			FormatLibrary::StandardLibrary::FormatTo( a_log_info, "Version: {0}\n", m_app_version );
			LogPrint( "norm", 1, "<SYSTEM_START>", a_log_info );
			FormatLibrary::StandardLibrary::FormatTo( a_log_info, "System started on: {0}", m_start_time );
			LogPrint( "norm", 1, "<SYSTEM_START>", a_log_info );
			FormatLibrary::StandardLibrary::FormatTo( w_log_info, L"Computer name: {0}.  Account name: {1}\n", computer_name, account_name );
			LogPrint( "norm", 1, "<SYSTEM_START>", w_log_info );
		}

		//CloseHandle( std_handle );
	}

	void SysLog::LogPrint( std::string log_kind, int32_t log_class, std::string log_cate, std::string log_info ) {
		switch( log_class ) {
		case 0: // debug
			SetColor( 8 ); break;
		case 1: // norm
			SetColor( 7 ); break;
		case 2: // prom
			SetColor( 3 ); break;
		case 3: // warn
			SetColor( 6 ); break;
		case 4: // error
			SetColor( 5 ); break;
		case 5: // death
			SetColor( 4 ); break;
		default:
			SetColor( 7 );
		}
		if( "none_endl" == log_kind ) {
			printf( "%s", log_info.c_str() );
		}
		else {
			printf( "%s\n", log_info.c_str() );
		}
	}

	void SysLog::LogPrint( std::string log_kind, int32_t log_class, std::string log_cate, std::wstring log_info ) {
		int32_t number = WideCharToMultiByte( CP_OEMCP, NULL, log_info.c_str(), -1, NULL, 0, NULL, FALSE );
		char* temp = new char[number];
		WideCharToMultiByte( CP_OEMCP, NULL, log_info.c_str(), -1, temp, number, NULL, FALSE );
		LogPrint( log_kind, log_class, log_cate, temp );
		delete [] temp;
	}

	void SysLog::LogWrite( std::string& log_kind, int32_t log_class, std::string& log_cate, std::string& log_info ) {
		//std::string log_text;
		//FormatLibrary::StandardLibrary::FormatTo( log_text, "{0} {1} {2} - {3}", log_kind, log_class, log_cate, log_info );
		//BOOST_LOG_SEV( *m_log_n, debug_n ) << log_text;
		//BOOST_LOG_SEV( *m_log_n, debug_n ) << log_kind << " " << log_class << " " << log_cate << " - " << log_info; // ���ܱ� Format ��

		//std::string log_text;
		//FormatLibrary::StandardLibrary::FormatTo( log_text, "{0} {1} {2} - {3}", log_kind, log_class, log_cate, log_info );
		//LOG( INFO ) << log_text;
		//LOG( INFO ) << log_kind << " " << log_class << " " << log_cate << " - " << log_info; // ���ܱ� Format ��

		//std::string log_text;
		//FormatLibrary::StandardLibrary::FormatTo( log_text, "{0} {1} {2} - {3}", log_kind, log_class, log_cate, log_info );
		//m_logger_sy->info( log_text );
		//m_logger_sy->info( "{} {} {} - {}", log_kind, log_class, log_cate, log_info ); // ���ܱ� Format ��
		m_logger_sy->info( log_info );

		//std::string log_text;
		//FormatLibrary::StandardLibrary::FormatTo( log_text, "{0} {1} {2} - {3}", log_kind, log_class, log_cate, log_info );
		//m_logger_as->info( log_text );
		//m_logger_as->info( "{} {} {} - {}", log_kind, log_class, log_cate, log_info ); // ���ܱ� Format ��
		//m_logger_as->info( log_info );
	}

} // namespace openex
