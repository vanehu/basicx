/*
 * Copyright (c) 2017-2018 the BasicX authors
 * All rights reserved.
 *
 * The project sponsor and lead author is Xu Rendong.
 * E-mail: xrd@ustc.edu, QQ: 277195007, WeChat: ustc_xrd
 * See the contributors file for names of other contributors.
 *
 * Commercial use of this code in source and binary forms is governed by
 * a LGPL v3 license. You may get a copy from project's root directory
 * and the official website page "www.gnu.org/licenses/lgpl.html".
 * Or else you should get a specific written permission.
 *
 * Individual and educational use of this code in source and binary forms
 * is governed by the following 3-clause BSD license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYSLOG_SYSLOG_H
#define SYSLOG_SYSLOG_H

#include <ctime>
#include <stdint.h> // int32_t, int64_t

#include <boost/log/sinks.hpp>
#include <boost/log/common.hpp>

#include "SpdLog/spdlog.h"

namespace openex {

	enum severity_level_n { debug_n, norm_n, prom_n, warn_n, error_n, death_n, start_n };
	enum severity_level_e { debug_e, norm_e, prom_e, warn_e, error_e, death_e, start_e };

	template<typename CharT, typename TraitsT>
	inline std::basic_ostream<CharT, TraitsT>& operator << ( std::basic_ostream<CharT, TraitsT>& strm, severity_level_n lvl ) {
		static const char* const str[] = { "debug_n", "norm_n", "prom_n", "warn_n", "error_n", "death_n", "start_n" };
		if( static_cast<std::size_t>( lvl ) < ( sizeof( str ) / sizeof( *str ) ) ) {
			strm << str[lvl];
		}
		else {
			strm << static_cast<int32_t>( lvl );
		}
		return strm;
	}

	template<typename CharT, typename TraitsT>
	inline std::basic_ostream<CharT, TraitsT>& operator << ( std::basic_ostream<CharT, TraitsT>& strm, severity_level_e lvl ) {
		static const char* const str[] = { "debug_e", "norm_e", "prom_e", "warn_e", "error_e", "death_e", "start_e" };
		if( static_cast<std::size_t>( lvl ) < ( sizeof( str ) / sizeof( *str ) ) ) {
			strm << str[lvl];
		}
		else {
			strm << static_cast<int32_t>( lvl );
		}
		return strm;
	}

	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> text_sink; // 性能点 - 1
	//typedef boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend> text_sink; // 性能点 - 1

	class SysLog
	{
	public:
		SysLog();
		~SysLog();

	public:
		static SysLog* GetInstance();

	public:
		void SetColor( unsigned short color );
		void InitSysLog( std::string app_name, std::string app_version );
		void ClearScreen( short row, short col, bool is_head = false, int32_t wait = 0 ); // 等待，毫秒
		// -1：启动(start)，0：调试(debug)，1：普通(norm)，2：提示(prom)，3：警告(warn)，4：错误(error)，5：致命(death)
		void LogPrint( std::string log_kind, int32_t log_class, std::string log_cate, std::string log_info );
		void LogPrint( std::string log_kind, int32_t log_class, std::string log_cate, std::wstring log_info );
		void LogWrite( std::string& log_kind, int32_t log_class, std::string& log_cate, std::string& log_info );

	private:
		std::string m_start_time;
		std::string m_log_folder;
		std::string m_app_name;
		std::string m_app_version;
		
		boost::mutex m_sink_lock;
		boost::shared_ptr<text_sink> m_sink_n;

		boost::log::sources::severity_logger<severity_level_n>* m_log_n;

		tm m_now_time_tm;
		std::time_t m_now_time_t;

		std::shared_ptr<spdlog::logger> m_logger_sy;
		std::shared_ptr<spdlog::sinks::daily_file_sink_st> m_sink_as_st;
		std::shared_ptr<spdlog::sinks::daily_file_sink_mt> m_sink_as_mt;
		std::shared_ptr<spdlog::async_logger> m_logger_as;

	private:
		static SysLog* m_instance;
	};

} // namespace openex

#endif // SYSLOG_SYSLOG_H
