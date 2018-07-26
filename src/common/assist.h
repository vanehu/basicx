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

#ifndef BASICX_COMMON_ASSIST_H
#define BASICX_COMMON_ASSIST_H

#include <ctime>
#include <tchar.h>
#include <stdint.h> // int32_t, int64_t
#include <iostream>

#include "sysdef.h"

#ifdef __OS_WINDOWS__
#include <windows.h>
#endif

namespace basicx {

	inline tm GetNowTime() {
		tm now_time;
		time_t now_time_t;
		time( &now_time_t );
		localtime_s( &now_time, &now_time_t );
		return now_time;
	}

	inline double Round( const double value, const size_t places ) {
		double result = 0.0;
		double module = value >= 0.0 ? 0.0000001 : -0.0000001;
		result = value;
		result += 5.0 / pow( 10.0, places + 1.0 );
		result *= pow( 10.0, places );
		result = floor( result + module );
		result /= pow( 10.0, places );
		return result;
	}

	inline double NearPrice( const double input_price, const double min_price_unit, const size_t price_places ) {
		int64_t price_temp = (int64_t)std::floor( input_price * std::pow( 10.0, price_places + 1 ) ); // 多保留一位，之后的截掉
		int64_t min_price_unit_temp = (int64_t)( min_price_unit * std::pow( 10.0, price_places + 1 ) );
		int64_t remainder_temp = price_temp % min_price_unit_temp;
		double min_price_unit_temp_half = (double)min_price_unit_temp / 2.0;
		// 下面比较大小，有可能均价刚好处于一个最小报价单位的中间，比如报价单位 0.05 均价 10.025 而上下两个价格为 10.05 和 10.00， 这里就用 >= 判断了，四舍五入嘛
		price_temp = remainder_temp >= min_price_unit_temp_half ? ( price_temp - remainder_temp + min_price_unit_temp ) : ( price_temp - remainder_temp );
		double output_price = (double)price_temp / std::pow( 10.0, price_places + 1 );
		// std::cout << std::setprecision( 6 ) << input_price << "：" << output_price << std::endl;
		return output_price;
	}

	inline std::string StringLeftTrim( const std::string& source, const std::string& drop ) {
		std::string result( source );
		return result.erase( 0, result.find_first_not_of( drop ) );
	}

	inline std::string StringRightTrim( const std::string& source, const std::string& drop ) {
		std::string result( source );
		return result.erase( result.find_last_not_of( drop ) + 1 );
	}

	inline std::string StringTrim( const std::string& source, const std::string& drop ) {
		return StringLeftTrim( StringRightTrim( source, drop ), drop );
	}

	inline void StringReplace( std::string& str_str, const std::string& str_src, const std::string& str_dst ) {
		std::string::size_type pos = 0;
		std::string::size_type src_len = str_src.size();
		std::string::size_type dst_len = str_dst.size();
		while( ( pos = str_str.find( str_src, pos ) ) != std::string::npos ) {
			str_str.replace( pos, src_len, str_dst );
			pos += dst_len;
		}
	}

	inline void AnsiCharToWideChar( const char* source, std::wstring& result ) {
#ifdef __OS_WINDOWS__
		int32_t ansi_length = MultiByteToWideChar( 0, 0, source, -1, NULL, 0 );
		wchar_t* temp_wide = new wchar_t[ansi_length + 1]; // 不加也行
		memset( temp_wide, 0, ansi_length * 2 + 2 );
		MultiByteToWideChar( 0, 0, source, -1, temp_wide, ansi_length );
		result = std::wstring( temp_wide );
		delete[] temp_wide;
#endif
	}

	inline void WideCharToAnsiChar( const wchar_t* source, std::string& result ) {
#ifdef __OS_WINDOWS__
		int32_t wide_length = WideCharToMultiByte( CP_OEMCP, NULL, source, -1, NULL, 0, NULL, FALSE );
		char* temp_ansi = new char[wide_length + 2]; // 不加也行
		memset( temp_ansi, 0, wide_length + 2 );
		WideCharToMultiByte( CP_OEMCP, NULL, source, -1, temp_ansi, wide_length, NULL, FALSE );
		result = std::string( temp_ansi );
		delete[] temp_ansi;
#endif
	}

	inline void GB2312toUTF8( const char* source, std::string& result ) {
#ifdef __OS_WINDOWS__
		int32_t unicode_length = MultiByteToWideChar( CP_ACP, 0, source, -1, NULL, 0 );
		wchar_t* temp_unicode = new wchar_t[unicode_length + 1];
		memset( temp_unicode, 0, unicode_length * 2 + 2 );
		MultiByteToWideChar( CP_ACP, 0, source, -1, temp_unicode, unicode_length ); // GB2312 to Unicode
		int32_t utf8_length = WideCharToMultiByte( CP_UTF8, 0, temp_unicode, -1, NULL, 0, NULL, NULL );
		char* temp_utf8 = new char[utf8_length + 1];
		memset( temp_utf8, 0, utf8_length + 1 );
		WideCharToMultiByte( CP_UTF8, 0, temp_unicode, -1, temp_utf8, utf8_length, NULL, NULL ); // Unicode to UTF8
		result = std::string( temp_utf8 );
		delete[] temp_utf8;
		delete[] temp_unicode;
#endif
	}

	inline void UTF8toGB2312( const char* source, std::string& result ) {
#ifdef __OS_WINDOWS__
		int32_t unicode_length = MultiByteToWideChar( CP_UTF8, 0, source, -1, NULL, 0 );
		wchar_t* temp_unicode = new wchar_t[unicode_length + 1];
		memset( temp_unicode, 0, unicode_length * 2 + 2 );
		MultiByteToWideChar( CP_UTF8, 0, source, -1, temp_unicode, unicode_length ); // UTF8 to Unicode
		int32_t gb2312_length = WideCharToMultiByte( CP_ACP, 0, temp_unicode, -1, NULL, 0, NULL, NULL );
		char* temp_gb2312 = new char[gb2312_length + 1];
		memset( temp_gb2312, 0, gb2312_length + 1 );
		WideCharToMultiByte( CP_ACP, 0, temp_unicode, -1, temp_gb2312, gb2312_length, NULL, NULL ); // Unicode to GB2312
		result = std::string( temp_gb2312 );
		delete[] temp_gb2312;
		delete[] temp_unicode;
#endif
	}

	inline std::wstring StringToWideChar( const std::string& source ) {
		std::wstring result = L"";
		AnsiCharToWideChar( source.c_str(), result );
		return result;
	}

	inline std::string StringToAnsiChar( const std::wstring& source ) {
		std::string result = "";
		WideCharToAnsiChar( source.c_str(), result );
		return result;
	}

	inline std::string StringToUTF8( const std::string& source ) {
		std::string result = "";
		GB2312toUTF8( source.c_str(), result );
		return result;
	}

	inline std::string StringToGB2312( const std::string& source ) {
		std::string result = "";
		UTF8toGB2312( source.c_str(), result );
		return result;
	}

	inline std::wstring StringToWideChar( const char* source ) {
		return StringToWideChar( std::string( source ) );
	}

	inline std::string StringToAnsiChar( const wchar_t* source ) {
		return StringToAnsiChar( std::wstring( source ) );
	}

	inline std::string StringToUTF8( const char* source ) {
		return StringToUTF8( std::string( source ) );
	}

	inline std::string StringToGB2312( const char* source ) {
		return StringToGB2312( std::string( source ) );
	}

	inline int64_t GetPerformanceFrequency() {
#ifdef __OS_WINDOWS__
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency( &frequency ); // 每秒跳动次数
		return frequency.QuadPart;
#endif
	}

	inline int64_t GetPerformanceTickCount() {
#ifdef __OS_WINDOWS__
		LARGE_INTEGER tick_count;
		QueryPerformanceCounter( &tick_count ); // 当前跳动次数
		return tick_count.QuadPart;
#endif
	}

	// windows：
	// THREAD_PRIORITY_ERROR_RETURN (MAXLONG) 0x7FFFFFFF 2147483647
	// THREAD_PRIORITY_TIME_CRITICAL == THREAD_BASE_PRIORITY_LOWRT 15 // value that gets a thread to LowRealtime-1
	// THREAD_PRIORITY_HIGHEST == THREAD_BASE_PRIORITY_MAX 2 // maximum thread base priority boost
	// THREAD_PRIORITY_ABOVE_NORMAL (THREAD_PRIORITY_HIGHEST-1)
	// THREAD_PRIORITY_NORMAL 0
	// THREAD_PRIORITY_BELOW_NORMAL (THREAD_PRIORITY_LOWEST+1)
	// THREAD_PRIORITY_LOWEST == THREAD_BASE_PRIORITY_MIN (-2) // minimum thread base priority boost
	// THREAD_PRIORITY_IDLE == THREAD_BASE_PRIORITY_IDLE (-15) // value that gets a thread to idle

	inline void SetThreadPriority( const int32_t thread_priority ) {
		::SetThreadPriority( GetCurrentThread(), thread_priority );
	}

	inline bool BindProcess( const size_t processor_number ) {
#ifdef __OS_WINDOWS__
		if( processor_number >= 1 && processor_number <= 32 ) {
			typedef void (CALLBACK* ULPRET)(SYSTEM_INFO*);
			ULPRET proc_address;
			HINSTANCE library;
			SYSTEM_INFO system_info;
			library = LoadLibraryA( "kernel32.dll" );
			if( library ) {
				proc_address = (ULPRET)GetProcAddress( library, "GetNativeSystemInfo" );
				if( proc_address ) { // 可以用 GetNativeSystemInfo，但是32位程序中 ActiveProcessorMask 最大还是只能32位即4294967295
					(*proc_address)(&system_info); // 等同于 GetNativeSystemInfo( &system_info );
				}
				else {
					GetSystemInfo( &system_info ); // 在64位系统，NumberOfProcessors 最大 32 个，ProcessorArchitecture 为 0 不正确
				}
				FreeLibrary( library );
			}

			// std::cout << "处理器：线程 = " << system_info.dwNumberOfProcessors << ", 活动 = " << system_info.dwActiveProcessorMask << ", 水平 = " << system_info.wProcessorLevel << ", 架构 = " << system_info.wProcessorArchitecture << ", 分页 = " << system_info.dwPageSize << "\n";

			if( processor_number > (int32_t)system_info.dwNumberOfProcessors ) {
				// std::cout << "进程绑定处理器时，核心线程编号 " << processor_number << " 超过 核心线程数 " << system_info.dwNumberOfProcessors << " 个！" << "\n";
				return false;
			}
			else {
				DWORD temp = 0x0001;
				DWORD mask = 0x0000;
				for( int32_t i = 0; i < 32; i++ ) { // 32位 0x0000
					if( ( i + 1 ) == processor_number ) {
						mask = temp;
						break;
					}
					temp *= 2; // 前移一位
				}

				// if( SetProcessAffinityMask( GetCurrentProcess(), mask ) ) {
				if( SetThreadAffinityMask( GetCurrentThread(), mask ) ) {
					// std::cout << "线程绑定至第 " << processor_number << " 个核心线程完成。" << "\n" << "\n";
					return true;
				}
				else {
					// std::cout << "线程绑定至第 " << processor_number << " 个核心线程失败！" << "\n" << "\n";
					return false;
				}
			}
		}
		return false;
#endif
	}

	inline void SetMinimumTimerResolution() {
#ifdef __OS_WINDOWS__
		typedef uint32_t* ( WINAPI* lpNST )( uint32_t, bool, uint32_t* );
		typedef uint32_t* ( WINAPI* lpNQT )( uint32_t*, uint32_t*, uint32_t* );
		HMODULE library = LoadLibrary( TEXT( "ntdll.dll" ) );
		if( library ) {
			lpNST NtSetTimerResolution = (lpNST)GetProcAddress( library, "NtSetTimerResolution" );
			lpNQT NtQueryTimerResolution = (lpNQT)GetProcAddress( library, "NtQueryTimerResolution" );
			if( nullptr == NtQueryTimerResolution || nullptr == NtSetTimerResolution ) {
				printf( "SetMinimumTimerResolution: Search function failed!\n" );
			}
			else {
				uint32_t max = 0;
				uint32_t min = 0;
				uint32_t cur = 0;
				uint32_t ret = 0;
				if( 0 == NtQueryTimerResolution( &max, &min, &cur ) ) {
					NtSetTimerResolution( min, true, &ret );
				}
			}
			FreeLibrary( library );
		}
#endif
	}

} // namespace basicx

#endif // BASICX_COMMON_ASSIST_H
