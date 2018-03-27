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

#ifndef BASICX_SYSLOG_SYSLOG_P_H
#define BASICX_SYSLOG_SYSLOG_P_H

#include <mutex>
#include <atomic>
#include <vector>
#include <fstream>
#include <stdint.h> // int32_t, int64_t

#include "syslog.h"

// C库缓冲 -- fflush --> 内核缓冲 -- fsync --> 磁盘
// 函数 int fd = fileno( FILE* stream ) 用来取得文件流所使用的文件描述符

namespace basicx {

	class LogItem
	{
	private:
		LogItem() {};

	public:
		LogItem( uint64_t log_id, syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move );
		~LogItem();

	public:
		void Update( uint64_t log_id, syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move );

	public:
		uint64_t m_log_id;
		std::chrono::system_clock::time_point m_log_time;
		syslog_level m_log_level;
		std::string m_log_cate;
		std::string m_log_info;
	};

	class LogVector
	{
	private:
		LogVector() {};

	public:
		LogVector( uint32_t capacity );
		~LogVector();

	public:
		std::atomic<uint32_t> m_count;
		std::atomic<uint32_t> m_handled;
		std::atomic<uint32_t> m_capacity;
		std::vector<LogItem*> m_vec_log_items;
	};

	class LogCacher
	{
	private:
		LogCacher() {};

	public:
		LogCacher( uint32_t cacher_id, uint32_t cacher_type, bool thread_safe, bool active_flush, bool active_sync, uint32_t capacity );
		~LogCacher();

	public:
		void HandleLogItemsPrint();
		void HandleLogItemsWrite();
		void LogPrint( LogItem* log_item );
		void LogWrite( LogItem* log_item );
		void SetTextColor( unsigned short color );
		void AddLogItem( uint64_t log_id, syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move );

	public:
		uint32_t m_cacher_id;
		uint32_t m_cacher_type;
		bool m_thread_safe;
		bool m_active_flush;
		bool m_active_sync;
		uint32_t m_capacity;
		bool m_fs_buffer_user; // 外部赋值
		int32_t m_fs_buffer_mode; // 外部赋值
		size_t m_fs_buffer_size; // 外部赋值
		int64_t m_log_days; // 外部赋值
		std::string m_log_name; // 外部赋值
		std::string m_log_path; // 外部赋值
		std::string m_log_folder; // 外部赋值

		FILE* m_log_file; // 外部赋值
		std::thread m_worker;
		std::atomic<bool> m_running;
		std::mutex m_worker_lock;
		std::unique_lock<std::mutex> m_unique_lock;
		std::condition_variable m_worker_cond;

		LogVector* m_log_vector_1;
		LogVector* m_log_vector_2;
		LogVector* m_log_vector_read;
		LogVector* m_log_vector_write;
		std::mutex m_writing_vector_lock;
		std::mutex m_changing_vector_lock;
	};

	class SysLog_P
	{
	private:
		SysLog_P() {};
		
	public:
		SysLog_P( std::string log_name );
		~SysLog_P();

	public:
		void SetThreadSafe( bool thread_safe ); // 默认 true
		void SetLocalCache( bool local_cache ); // 默认 true
		void SetActiveFlush( bool active_flush ); // 默认 false
		void SetActiveSync( bool active_sync ); // 默认 false
		void SetWorkThreads( size_t work_threads ); // 默认 1
		void SetInitCapacity( uint32_t init_capacity ); // 默认 8192
		void SetFileStreamBuffer( int32_t mode, size_t size = 0 ); // 会影响性能，但在静态链接 MySQL、MariaDB 时需要设为 无缓冲 不然写入文件的日志会被缓存
		void InitSysLog( std::string app_name, std::string app_version, std::string app_company, std::string app_copyright ); // 设置好参数后再调用
		void PrintSysInfo();
		void WriteSysInfo();
		void SetTextColor( unsigned short color );
		void ClearScreen( short row, short col, bool print_head = false, int32_t wait = 0 ); // 等待，毫秒
		// 0：调试(debug)，1：信息(info)，2：提示(hint)，3：警告(warn)，4：错误(error)，5：致命(fatal)
		void LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move = false );
		void LogWrite( syslog_level log_level, std::string& log_cate, std::string& log_info, bool log_move = false );

	private:
		bool m_thread_safe;
		bool m_local_cache;
		bool m_active_flush;
		bool m_active_sync;
		size_t m_work_threads;
		uint32_t m_init_capacity;
		bool m_fs_buffer_user;
		int32_t m_fs_buffer_mode;
		size_t m_fs_buffer_size;

		int64_t m_log_days;
		std::string m_log_name;
		std::string m_log_cate;
		std::string m_log_path;
		std::string m_log_folder;
		std::string m_app_name;
		std::string m_app_version;
		std::string m_app_company;
		std::string m_app_copyright;
		std::string m_start_time;
		std::string m_account_name;
		std::string m_computer_name;
		
		FILE* m_log_file;

		std::mutex m_print_log_lock;
		std::mutex m_build_file_lock;
		std::mutex m_write_file_lock;

		std::atomic<uint64_t> m_log_item_id;
		std::vector<LogCacher*> m_vec_log_cachers;

		LogCacher* m_log_cacher_print;
	};

} // namespace basicx

#endif // BASICX_SYSLOG_SYSLOG_P_H
