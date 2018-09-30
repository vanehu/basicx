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

#include <ctime>
#include <chrono>
#include <thread>
#include <share.h> // _fsopen
#include <iostream>

#include <common/sysdef.h>
#include <common/assist.h>
#include <common/Format/Format.hpp>

#ifdef __OS_WINDOWS__
#include <windows.h>
#endif

#include "syslog_.h"

#define DEF_SYSLOG_LOGCACHER_PRINT 1
#define DEF_SYSLOG_LOGCACHER_WRITE 2

namespace basicx {

	LogItem::LogItem( uint64_t log_id, syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move )
	: m_log_time( std::chrono::system_clock::now() )
	, m_log_id( log_id )
	, m_log_level( log_level )
	, m_log_cate( log_cate ) {
		if( true == log_move ) {
			m_log_info = std::move( log_info );
		}
		else {
			m_log_info = log_info;
		}
	}

	LogItem::~LogItem() {
	}

	void LogItem::Update( uint64_t log_id, syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move ) {
		m_log_time = std::chrono::system_clock::now();
		m_log_id = log_id;
		m_log_level = log_level;
		m_log_cate = log_cate;
		if( true == log_move ) {
			m_log_info = std::move( log_info );
		}
		else {
			m_log_info = log_info;
		}
	}

	LogVector::LogVector( uint32_t capacity )
		: m_count( 0 )
		, m_handled( 0 )
		, m_capacity( capacity ) {
	}

	LogVector::~LogVector() {
		for( size_t i = 0; i < m_vec_log_items.size(); i++ ) {
			if( m_vec_log_items[i] != nullptr ) {
				delete m_vec_log_items[i];
			}
		}
		m_vec_log_items.clear();
	}

	LogCacher::LogCacher( uint32_t cacher_id, uint32_t cacher_type, bool thread_safe, bool active_flush, bool active_sync, uint32_t capacity )
		: m_cacher_id( cacher_id )
		, m_cacher_type( cacher_type )
		, m_thread_safe( thread_safe )
		, m_active_flush( active_flush )
		, m_active_sync( active_sync )
		, m_capacity( capacity )
		, m_fs_buffer_user( false )
		, m_fs_buffer_mode( _IONBF )
		, m_fs_buffer_size( 4096 )
		, m_log_vector_1( nullptr )
		, m_log_vector_2( nullptr )
		, m_log_vector_read( nullptr )
		, m_log_vector_write( nullptr )
		, m_log_days( 0 )
		, m_log_name( "" )
		, m_log_path( "" )
		, m_log_folder( "" )
		, m_log_file( nullptr )
		, m_unique_lock( m_worker_lock ) {
		m_log_vector_1 = new LogVector( capacity );
		m_log_vector_2 = new LogVector( capacity );
		m_log_vector_read = m_log_vector_1;
		m_log_vector_write = m_log_vector_1;
		m_running = true;
		if( DEF_SYSLOG_LOGCACHER_PRINT == m_cacher_type ) {
			m_worker = std::thread( &LogCacher::HandleLogItemsPrint, this );
		}
		if( DEF_SYSLOG_LOGCACHER_WRITE == m_cacher_type ) {
			m_worker = std::thread( &LogCacher::HandleLogItemsWrite, this );
		}
	}

	LogCacher::~LogCacher() {
		m_running = false;
		m_worker_cond.notify_all();
		m_worker.join();
		if( m_log_file != nullptr ) {
			fflush( m_log_file );
			fclose( m_log_file );
			m_log_file = nullptr;
		}
		if( m_log_vector_1 != nullptr ) {
			delete m_log_vector_1;
		}
		if( m_log_vector_2 != nullptr ) {
			delete m_log_vector_2;
		}
	}

	void LogCacher::HandleLogItemsPrint() {
		while( true == m_running ) {
			m_worker_cond.wait( m_unique_lock );
			for( ; m_log_vector_read->m_handled < m_log_vector_read->m_count; ) {
				m_log_vector_read->m_handled++;
				LogPrint( m_log_vector_read->m_vec_log_items[m_log_vector_read->m_handled - 1] );
			}
			while( m_log_vector_read->m_handled == m_log_vector_read->m_capacity ) { // 需换队列，必需读满 capacity 才考虑更换
				bool changed = false;
				m_changing_vector_lock.lock();
				if( m_log_vector_read != m_log_vector_write ) { // 写线程已在目标队列，允许切换
					m_log_vector_read = m_log_vector_read == m_log_vector_1 ? m_log_vector_2 : m_log_vector_1; // 切换队列
					m_log_vector_read->m_handled = 0; // 重新开始所在队列计数
					changed = true; //
				}
				// else {} // 等待写线程跳到目标队列
				m_changing_vector_lock.unlock();
				if( true == changed ) { // 已换到新队列
					for( ; m_log_vector_read->m_handled < m_log_vector_read->m_count; ) {
						m_log_vector_read->m_handled++;
						LogPrint( m_log_vector_read->m_vec_log_items[m_log_vector_read->m_handled - 1] );
					}
				}
				// 在处理日志过程中，写线程可能又把队列写满了，所以查验是否继续切换
			}
		}
	}

	void LogCacher::HandleLogItemsWrite() {
		while( true == m_running ) {
			m_worker_cond.wait( m_unique_lock );
			for( ; m_log_vector_read->m_handled < m_log_vector_read->m_count; ) {
				m_log_vector_read->m_handled++;
				LogWrite( m_log_vector_read->m_vec_log_items[m_log_vector_read->m_handled - 1] );
			}
			while( m_log_vector_read->m_handled == m_log_vector_read->m_capacity ) { // 需换队列，必需读满 capacity 才考虑更换
				bool changed = false;
				m_changing_vector_lock.lock();
				if( m_log_vector_read != m_log_vector_write ) { // 写线程已在目标队列，允许切换
					m_log_vector_read = m_log_vector_read == m_log_vector_1 ? m_log_vector_2 : m_log_vector_1; // 切换队列
					m_log_vector_read->m_handled = 0; // 重新开始所在队列计数
					changed = true; //
				}
				// else {} // 等待写线程跳到目标队列
				m_changing_vector_lock.unlock();
				if( true == changed ) { // 已换到新队列
					for( ; m_log_vector_read->m_handled < m_log_vector_read->m_count; ) {
						m_log_vector_read->m_handled++;
						LogWrite( m_log_vector_read->m_vec_log_items[m_log_vector_read->m_handled - 1] );
					}
				}
				// 在处理日志过程中，写线程可能又把队列写满了，所以查验是否继续切换
			}
		}
	}

	void LogCacher::LogPrint( LogItem* log_item ) {
		switch( log_item->m_log_level ) {
		case syslog_level::n_debug: case syslog_level::c_debug:
			SetTextColor( 8 ); break;
		case syslog_level::n_info:  case syslog_level::c_info:
			SetTextColor( 7 ); break;
		case syslog_level::n_hint:  case syslog_level::c_hint:
			SetTextColor( 3 ); break;
		case syslog_level::n_warn:  case syslog_level::c_warn:
			SetTextColor( 6 ); break;
		case syslog_level::n_error: case syslog_level::c_error:
			SetTextColor( 5 ); break;
		case syslog_level::n_fatal: case syslog_level::c_fatal:
			SetTextColor( 4 ); break;
		default:
			SetTextColor( 7 ); break;
		}
		// 这里打日志是单线程的，不用为打印日志加锁
		printf( "%s\n", log_item->m_log_info.c_str() );
	}

	void LogCacher::LogWrite( LogItem* log_item ) {
		time_t now_time_t = std::chrono::system_clock::to_time_t( std::chrono::floor<std::chrono::seconds>( log_item->m_log_time ) ); // 下取整
		int64_t now_days = std::chrono::floor<std::chrono::duration<int64_t, std::ratio<86400>>>( log_item->m_log_time ).time_since_epoch().count(); // 天数
		if( now_days != m_log_days ) {
			// 这里写日志是单线程的，不用为创建新文件加锁
			m_log_days = now_days; //
			tm now_file_tm = { 0 };
			char now_file_buf[20] = { 0 };
			localtime_s( &now_file_tm, &now_time_t );
			strftime( now_file_buf, 20, "%Y-%m-%d_%H-%M-%S", &now_file_tm );
			FormatLibrary::StandardLibrary::FormatTo( m_log_path, "{0}\\{1}_{2}_{3}.log", m_log_folder, m_log_name, m_cacher_id, now_file_buf ); // cacher_id
			if( m_log_file != nullptr ) {
				fflush( m_log_file );
				fclose( m_log_file );
				m_log_file = nullptr;
			}
			//fopen_s( &m_log_file, m_log_path.c_str(), "wb" );
			m_log_file = _fsopen( m_log_path.c_str(), "wb", _SH_DENYNO );
			if( true == m_fs_buffer_user ) {
				setvbuf( m_log_file, nullptr, m_fs_buffer_mode, m_fs_buffer_size );
			}
		}
		tm now_time_tm = { 0 };
		char log_numb_buf[10] = { 0 };
		char now_time_buf[20] = { 0 };
		char the_nano_buf[10] = { 0 };
		localtime_s( &now_time_tm, &now_time_t );
		snprintf( log_numb_buf, 10, "%09lld", log_item->m_log_id + 1 );
		strftime( now_time_buf, 20, "%Y-%m-%d %H:%M:%S", &now_time_tm );
		snprintf( the_nano_buf, 10, "%09lld", std::chrono::duration_cast<std::chrono::nanoseconds>( log_item->m_log_time - std::chrono::system_clock::from_time_t( now_time_t ) ).count() );
		std::string log_text;
		log_text.append( log_numb_buf );
		log_text.append( " " );
		log_text.append( now_time_buf );
		log_text.append( "." );
		log_text.append( the_nano_buf );
		log_text.append( " " );
		log_text.append( (const char*)&log_item->m_log_level );
		log_text.append( " " + log_item->m_log_cate + " - " + log_item->m_log_info + "\r\n" );
		// 这里写日志是单线程的，不用为写入日志加锁
		fwrite( log_text.c_str(), log_text.length(), 1, m_log_file );
		if( true == m_active_flush ) {
			fflush( m_log_file );
			if( true == m_active_sync ) {
#ifdef __OS_WINDOWS__
				FlushFileBuffers( m_log_file );
#endif
				// Linux 使用 fsync 且可用 fdatasync 减少磁盘操作改进性能
			}
		}
	}

	void LogCacher::SetTextColor( unsigned short color ) {
#ifdef __OS_WINDOWS__
		HANDLE std_handle = GetStdHandle( STD_OUTPUT_HANDLE ); // 获得缓冲区句柄
		SetConsoleTextAttribute( std_handle, color ); // 设置文本及背景颜色
		//CloseHandle( std_handle );
#endif
	}

	void LogCacher::AddLogItem( uint64_t log_id, syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move ) {
		if( true == m_thread_safe ) {
			m_writing_vector_lock.lock();
		}
		if( m_log_vector_write->m_count == m_log_vector_write->m_capacity ) { // 需换队列，必需写满 capacity 才考虑更换
			m_changing_vector_lock.lock();
			if( m_log_vector_write == m_log_vector_read ) { // 读线程未占用目标队列，允许切换
				m_log_vector_write = m_log_vector_write == m_log_vector_1 ? m_log_vector_2 : m_log_vector_1; // 切换队列
				m_log_vector_write->m_count = 0; // 重新开始所在队列计数
			}
			else {
				m_log_vector_write->m_capacity += m_capacity; // 增加点允许长度，等待读线程离开目标队列
			}
			m_changing_vector_lock.unlock();
		}
		if( m_log_vector_write->m_count < m_log_vector_write->m_vec_log_items.size() ) { // 使用旧有元素
			m_log_vector_write->m_vec_log_items[m_log_vector_write->m_count]->Update( log_id, log_level, log_cate, log_info, log_move );
			m_log_vector_write->m_count++; // 更新以后
		}
		else { // 使用新建元素
			m_log_vector_write->m_vec_log_items.push_back( new LogItem( log_id, log_level, log_cate, log_info, log_move ) );
			m_log_vector_write->m_count++; // 添加以后
		}
		m_worker_cond.notify_all(); //
		if( true == m_thread_safe ) {
			m_writing_vector_lock.unlock();
		}
	}

	SysLog_P::SysLog_P( std::string log_name )
		: m_log_days( 0 )
		, m_log_name( log_name )
		, m_thread_safe( true )
		, m_local_cache( true )
		, m_active_flush( false )
		, m_active_sync( false )
		, m_work_threads( 1 )
		, m_init_capacity( 8192 )
		, m_fs_buffer_user( false )
		, m_fs_buffer_mode( _IONBF )
		, m_fs_buffer_size( 4096 )
		, m_log_cate( "<SYSLOG>" )
		, m_log_path( "" )
		, m_log_folder( "" )
		, m_app_name( "" )
		, m_app_version( "" )
		, m_app_company( "" )
		, m_app_copyright( "" )
		, m_start_time( "" )
		, m_account_name( "" )
		, m_computer_name( "" )
		, m_log_file( nullptr )
		, m_log_item_id( 0 )
		, m_log_cacher_print( nullptr ) {
	}

	SysLog_P::~SysLog_P() {
		if( m_log_file != nullptr ) {
			fflush( m_log_file );
			fclose( m_log_file );
			m_log_file = nullptr;
		}
		for( size_t i = 0; i < m_vec_log_cachers.size(); i++ ) {
			delete m_vec_log_cachers[i];
		}
		if( m_log_cacher_print != nullptr ) {
			delete m_log_cacher_print;
		}
	}

	void SysLog_P::SetThreadSafe( bool thread_safe ) {
		m_thread_safe = thread_safe;
	}

	void SysLog_P::SetLocalCache( bool local_cache ) {
		m_local_cache = local_cache;
	}

	void SysLog_P::SetActiveFlush( bool active_flush ) {
		m_active_flush = active_flush;
	}

	void SysLog_P::SetActiveSync( bool active_sync ) {
		m_active_sync = active_sync;
	}

	void SysLog_P::SetWorkThreads( size_t work_threads ) {
		m_work_threads = work_threads;
	}

	void SysLog_P::SetInitCapacity( uint32_t init_capacity ) {
		m_init_capacity = init_capacity;
	}

	void SysLog_P::SetFileStreamBuffer( int32_t mode, size_t size/* = 0*/ ) {
		m_fs_buffer_user = true; // 只要调用了就设为 true
		m_fs_buffer_mode = mode;
		m_fs_buffer_size = size;
	}

	void SysLog_P::InitSysLog( std::string app_name, std::string app_version, std::string app_company, std::string app_copyright ) {
		m_app_name = app_name;
		m_app_version = app_version;
		m_app_company = app_company;
		m_app_copyright = app_copyright;

		std::time_t start_t = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
		tm start_tm;
		localtime_s( &start_tm, &start_t );
		char start_time_buf[64] = { 0 };
		strftime( start_time_buf, 64, "%Y-%m-%d %a %H:%M:%S", &start_tm );
		m_start_time = start_time_buf;

#ifdef __OS_WINDOWS__
		wchar_t account_name[64] = { 0 };
		wchar_t computer_name[64] = { 0 };
		DWORD size_account_name = 64;
		DWORD size_computer_name = 64;
		GetUserName( account_name, &size_account_name ); // 需要 Advapi32.lib
		GetComputerName( computer_name, &size_computer_name );
		m_account_name = StringToAnsiChar( account_name );
		m_computer_name = StringToAnsiChar( computer_name );

		wchar_t char_path[MAX_PATH] = { 0 };
		GetModuleFileName( NULL, char_path, MAX_PATH );
		std::wstring string_path( char_path );
		size_t slash_index = string_path.rfind( '\\' );
		//string_path = string_path.substr( 0, slash_index );
		//slash_index = string_path.rfind( '\\' );
		string_path = string_path.substr( 0, slash_index ) + L"\\logfiles";
		// 正常情况下程序运行时日志文件夹是删不掉的，所以这里建立以后不再检测该文件夹是否存在
		WIN32_FIND_DATA find_data;
		void* find_file = FindFirstFile( string_path.c_str(), &find_data );
		if( INVALID_HANDLE_VALUE == find_file ) {
			CreateDirectory( string_path.c_str(), NULL );
		}

		m_log_folder = StringToAnsiChar( string_path );
#endif

		//#include <boost/filesystem.hpp>
		//std::string module_folder = boost::filesystem::initial_path<boost::filesystem::path>().string();
		//size_t slash_index = module_folder.rfind( '\\' );
		//m_log_folder = module_folder.substr( 0, slash_index ) + "\\logfiles";
		//// 正常情况下程序运行时日志文件夹是删不掉的，所以这里建立以后不再检测该文件夹是否存在
		//if( !boost::filesystem::exists( m_log_folder ) ) {
		//	boost::filesystem::create_directories( m_log_folder );
		//}

		std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();
		time_t now_time_t = std::chrono::system_clock::to_time_t( std::chrono::floor<std::chrono::seconds>( now_time ) ); // 下取整
		m_log_days = std::chrono::floor<std::chrono::duration<int64_t, std::ratio<86400>>>( now_time ).time_since_epoch().count(); // 天数

		tm now_file_tm = { 0 };
		char now_file_buf[20] = { 0 };
		localtime_s( &now_file_tm, &now_time_t );
		strftime( now_file_buf, 20, "%Y-%m-%d_%H-%M-%S", &now_file_tm );

		if( true == m_local_cache ) {
			for( size_t i = 0; i < m_work_threads; i++ ) {
				LogCacher* cacher = new LogCacher( i + 1, DEF_SYSLOG_LOGCACHER_WRITE, m_thread_safe, m_active_flush, m_active_sync, m_init_capacity );
				cacher->m_fs_buffer_user = m_fs_buffer_user;
				cacher->m_fs_buffer_mode = m_fs_buffer_mode;
				cacher->m_fs_buffer_size = m_fs_buffer_size;
				cacher->m_log_days = m_log_days;
				cacher->m_log_name = m_log_name;
				FormatLibrary::StandardLibrary::FormatTo( cacher->m_log_path, "{0}\\{1}_{2}_{3}.log", m_log_folder, m_log_name, i + 1, now_file_buf ); // cacher_id
				cacher->m_log_folder = m_log_folder;
				//fopen_s( &cacher->m_log_file, cacher->m_log_path.c_str(), "wb" );
				cacher->m_log_file = _fsopen( cacher->m_log_path.c_str(), "wb", _SH_DENYNO );
				if( true == m_fs_buffer_user ) {
					setvbuf( cacher->m_log_file, nullptr, m_fs_buffer_mode, m_fs_buffer_size );
				}
				m_vec_log_cachers.push_back( cacher );
			}
			m_log_cacher_print = new LogCacher( 0, DEF_SYSLOG_LOGCACHER_PRINT, m_thread_safe, m_active_flush, m_active_sync, m_init_capacity ); //
		}
		else {
			FormatLibrary::StandardLibrary::FormatTo( m_log_path, "{0}\\{1}_{2}.log", m_log_folder, m_log_name, now_file_buf );
			//fopen_s( &m_log_file, m_log_path.c_str(), "wb" );
			m_log_file = _fsopen( m_log_path.c_str(), "wb", _SH_DENYNO );
			if( true == m_fs_buffer_user ) {
				setvbuf( m_log_file, nullptr, m_fs_buffer_mode, m_fs_buffer_size );
			}
		}
	}

	void SysLog_P::PrintSysInfo() {
		std::string log_info = "";
		FormatLibrary::StandardLibrary::FormatTo( log_info, "\nWelcome to {0}", m_app_name );
		LogPrint( syslog_level::n_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "Version: {0}\n", m_app_version );
		LogPrint( syslog_level::n_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "System started on: {0}", m_start_time );
		LogPrint( syslog_level::n_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "Computer name: {0}  Account name: {1}\n", m_computer_name, m_account_name );
		LogPrint( syslog_level::n_info, m_log_cate, log_info );
	}

	void SysLog_P::WriteSysInfo() {
		std::string log_info = "";
		log_info = "***********************************************************************************";
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "                            {0}", m_app_company );
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "                  {0}", m_app_copyright );
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		log_info = "***********************************************************************************";
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "系统：{0}", m_app_name );
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "版本：{0}", m_app_version );
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "启动：{0}", m_start_time );
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "电脑：{0}", m_computer_name );
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "用户：{0}", m_account_name );
		for( size_t i = 0; i < m_work_threads; i++ ) {
			LogWrite( syslog_level::c_info, m_log_cate, log_info );
		}
	}

	void SysLog_P::SetTextColor( unsigned short color ) {
#ifdef __OS_WINDOWS__
		HANDLE std_handle = GetStdHandle( STD_OUTPUT_HANDLE ); // 获得缓冲区句柄
		SetConsoleTextAttribute( std_handle, color ); // 设置文本及背景颜色
		//CloseHandle( std_handle );
#endif
	}

	void SysLog_P::ClearScreen( short row, short col, bool print_head/* = false*/, int32_t wait/* = 0*/ ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( wait ) );
#ifdef __OS_WINDOWS__
		//system( "cls" ); // 也可以简单的用这句清屏
		HANDLE std_handle = GetStdHandle( STD_OUTPUT_HANDLE );
		CONSOLE_SCREEN_BUFFER_INFO buffer_info; // 窗口信息
		GetConsoleScreenBufferInfo( std_handle, &buffer_info );
		COORD position = { col, row };
		SetConsoleCursorPosition( std_handle, position ); // 设置光标位置
		DWORD chars_written; // 在 Windows 10 下必须使用，而不能用 NULL 代替，否则会崩溃
		FillConsoleOutputCharacter( std_handle, ' ', buffer_info.dwSize.X * buffer_info.dwSize.Y, position, &chars_written );
		//CloseHandle( std_handle );
#endif
		if( true == print_head ) {
			PrintSysInfo();
		}
	}

	void SysLog_P::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move/* = false*/ ) {
		if( true == m_local_cache ) {
			m_log_cacher_print->AddLogItem( 0, log_level, log_cate, log_info, log_move );
		}
		else {
			switch( log_level ) {
			case syslog_level::n_debug: case syslog_level::c_debug:
				SetTextColor( 8 ); break;
			case syslog_level::n_info:  case syslog_level::c_info:
				SetTextColor( 7 ); break;
			case syslog_level::n_hint:  case syslog_level::c_hint:
				SetTextColor( 3 ); break;
			case syslog_level::n_warn:  case syslog_level::c_warn:
				SetTextColor( 6 ); break;
			case syslog_level::n_error: case syslog_level::c_error:
				SetTextColor( 5 ); break;
			case syslog_level::n_fatal: case syslog_level::c_fatal:
				SetTextColor( 4 ); break;
			default:
				SetTextColor( 7 ); break;
			}
#ifdef __OS_WINDOWS__
			if( true == m_thread_safe ) {
				m_print_log_lock.lock();
			}
#endif
			printf( "%s\n", log_info.c_str() );
#ifdef __OS_WINDOWS__
			if( true == m_thread_safe ) {
				m_print_log_lock.unlock();
			}
#endif
		}
	}

	// 在同一个进程内，针对同一个 FILE* 的操作（比如 fwrite），是线程安全的，当然这只在 POSIX 兼容的系统上成立，Windows 上的 FILE* 的操作并不是线程安全的
	void SysLog_P::LogWrite( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move/* = false*/ ) {
		if( true == m_local_cache ) {
			m_vec_log_cachers[m_log_item_id % m_work_threads]->AddLogItem( m_log_item_id, log_level, log_cate, log_info, log_move );
			m_log_item_id++;
		}
		else {
			std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();
			time_t now_time_t = std::chrono::system_clock::to_time_t( std::chrono::floor<std::chrono::seconds>( now_time ) ); // 下取整
			int64_t now_days = std::chrono::floor<std::chrono::duration<int64_t, std::ratio<86400>>>( now_time ).time_since_epoch().count(); // 天数
			if( now_days != m_log_days ) {
				m_build_file_lock.lock();
				if( now_days != m_log_days ) { // 二次检测
					m_log_days = now_days; //
					tm now_file_tm = { 0 };
					char now_file_buf[20] = { 0 };
					localtime_s( &now_file_tm, &now_time_t );
					strftime( now_file_buf, 20, "%Y-%m-%d_%H-%M-%S", &now_file_tm );
					FormatLibrary::StandardLibrary::FormatTo( m_log_path, "{0}\\{1}_{2}.log", m_log_folder, m_log_name, now_file_buf );
					if( m_log_file != nullptr ) {
						fflush( m_log_file );
						fclose( m_log_file );
						m_log_file = nullptr;
					}
					//fopen_s( &m_log_file, m_log_path.c_str(), "wb" );
					m_log_file = _fsopen( m_log_path.c_str(), "wb", _SH_DENYNO );
					if( true == m_fs_buffer_user ) {
						setvbuf( m_log_file, nullptr, m_fs_buffer_mode, m_fs_buffer_size );
					}
				}
				m_build_file_lock.unlock();
			}
			tm now_time_tm = { 0 };
			char now_time_buf[20] = { 0 };
			char the_nano_buf[10] = { 0 };
			localtime_s( &now_time_tm, &now_time_t );
			strftime( now_time_buf, 20, "%Y-%m-%d %H:%M:%S", &now_time_tm );
			snprintf( the_nano_buf, 10, "%09lld", std::chrono::duration_cast<std::chrono::nanoseconds>( now_time - std::chrono::system_clock::from_time_t( now_time_t ) ).count() );
			std::string log_text;
			log_text.append( now_time_buf );
			log_text.append( "." );
			log_text.append( the_nano_buf );
			log_text.append( " " );
			log_text.append( (const char*)&log_level );
			log_text.append( " " + log_cate + " - " + log_info + "\r\n" );
#ifdef __OS_WINDOWS__
			if( true == m_thread_safe ) {
				m_write_file_lock.lock();
			}
#endif
			fwrite( log_text.c_str(), log_text.length(), 1, m_log_file );
			if( true == m_active_flush ) {
				fflush( m_log_file );
				if( true == m_active_sync ) {
#ifdef __OS_WINDOWS__
					FlushFileBuffers( m_log_file );
#endif
					// Linux 使用 fsync 且可用 fdatasync 减少磁盘操作改进性能
				}
			}
#ifdef __OS_WINDOWS__
			if( true == m_thread_safe ) {
				m_write_file_lock.unlock();
			}
#endif
		}
	}

	SysLog_K::SysLog_K( std::string log_name )
		: m_syslog_p( nullptr ) {
		try {
			m_syslog_p = new SysLog_P( log_name );
		}
		catch( ... ) {}
	}

	SysLog_K::~SysLog_K() {
		if( m_syslog_p != nullptr ) {
			delete m_syslog_p;
			m_syslog_p = nullptr;
		}
	}

	SysLog_D::SysLog_D( std::string log_name )
		: m_syslog_p( nullptr ) {
		try {
			m_syslog_p = new SysLog_P( log_name );
		}
		catch( ... ) {}
	}

	SysLog_D::~SysLog_D() {
		if( m_syslog_p != nullptr ) {
			delete m_syslog_p;
			m_syslog_p = nullptr;
		}
	}

	void SysLog_D::SetThreadSafe( bool thread_safe ) {
		m_syslog_p->SetThreadSafe( thread_safe );
	}

	void SysLog_D::SetLocalCache( bool local_cache ) {
		m_syslog_p->SetLocalCache( local_cache );
	}

	void SysLog_D::SetActiveFlush( bool active_flush ) {
		m_syslog_p->SetActiveFlush( active_flush );
	}

	void SysLog_D::SetActiveSync( bool active_sync ) {
		m_syslog_p->SetActiveSync( active_sync );
	}

	void SysLog_D::SetWorkThreads( size_t work_threads ) {
		m_syslog_p->SetWorkThreads( work_threads );
	}

	void SysLog_D::SetInitCapacity( uint32_t init_capacity ) {
		m_syslog_p->SetInitCapacity( init_capacity );
	}

	void SysLog_D::SetFileStreamBuffer( int32_t mode, size_t size/* = 0*/ ) {
		m_syslog_p->SetFileStreamBuffer( mode, size );
	}

	void SysLog_D::InitSysLog( std::string app_name, std::string app_version, std::string app_company, std::string app_copyright ) {
		m_syslog_p->InitSysLog( app_name, app_version, app_company, app_copyright );
	}

	void SysLog_D::PrintSysInfo() {
		m_syslog_p->PrintSysInfo();
	}

	void SysLog_D::WriteSysInfo() {
		m_syslog_p->WriteSysInfo();
	}

	void SysLog_D::ClearScreen( short row, short col, bool print_head/* = false*/, int32_t wait/* = 0*/ ) {
		m_syslog_p->ClearScreen( row, col, print_head, wait );
	}

	void SysLog_D::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move/* = false*/ ) {
		m_syslog_p->LogPrint( log_level, log_cate, log_info, log_move );
	}

	void SysLog_D::LogWrite( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move/* = false*/ ) {
		m_syslog_p->LogWrite( log_level, log_cate, log_info, log_move );
	}

	basicx::SysLog_S* basicx::SysLog_S::m_instance = nullptr;

	SysLog_S::SysLog_S( std::string log_name )
		: m_syslog_p( nullptr ) {
		try {
			m_syslog_p = new SysLog_P( log_name );
		}
		catch( ... ) {}
		m_instance = this;
	}

	SysLog_S::~SysLog_S() {
		if( m_syslog_p != nullptr ) {
			delete m_syslog_p;
			m_syslog_p = nullptr;
		}
	}

	SysLog_S* SysLog_S::GetInstance() {
		return m_instance;
	}

	void SysLog_S::SetThreadSafe( bool thread_safe ) {
		m_syslog_p->SetThreadSafe( thread_safe );
	}

	void SysLog_S::SetLocalCache( bool local_cache ) {
		m_syslog_p->SetLocalCache( local_cache );
	}

	void SysLog_S::SetActiveFlush( bool active_flush ) {
		m_syslog_p->SetActiveFlush( active_flush );
	}

	void SysLog_S::SetActiveSync( bool active_sync ) {
		m_syslog_p->SetActiveSync( active_sync );
	}

	void SysLog_S::SetWorkThreads( size_t work_threads ) {
		m_syslog_p->SetWorkThreads( work_threads );
	}

	void SysLog_S::SetInitCapacity( uint32_t init_capacity ) {
		m_syslog_p->SetInitCapacity( init_capacity );
	}

	void SysLog_S::SetFileStreamBuffer( int32_t mode, size_t size/* = 0*/ ) {
		m_syslog_p->SetFileStreamBuffer( mode, size );
	}

	void SysLog_S::InitSysLog( std::string app_name, std::string app_version, std::string app_company, std::string app_copyright ) {
		m_syslog_p->InitSysLog( app_name, app_version, app_company, app_copyright );
	}

	void SysLog_S::PrintSysInfo() {
		m_syslog_p->PrintSysInfo();
	}

	void SysLog_S::WriteSysInfo() {
		m_syslog_p->WriteSysInfo();
	}

	void SysLog_S::ClearScreen( short row, short col, bool print_head/* = false*/, int32_t wait/* = 0*/ ) {
		m_syslog_p->ClearScreen( row, col, print_head, wait );
	}

	void SysLog_S::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move/* = false*/ ) {
		m_syslog_p->LogPrint( log_level, log_cate, log_info, log_move );
	}

	void SysLog_S::LogWrite( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move/* = false*/ ) {
		m_syslog_p->LogWrite( log_level, log_cate, log_info, log_move );
	}

} // namespace basicx
