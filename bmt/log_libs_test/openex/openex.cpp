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

#include <ctime>
//#include <tchar.h>
#include <fstream>
#include <iostream>

#include "syslog.h"

using namespace openex;

SysLog g_sys_log; // 唯一实例

void test_write() {
	const size_t log_test_times = 5000000;
	const char* c_plus_write_file_1 = "C:\\Users\\xrd\\Desktop\\openex\\bin\\c_plus_write_file_1.txt";
	const char* c_plus_write_file_2 = "C:\\Users\\xrd\\Desktop\\openex\\bin\\c_plus_write_file_2.txt";
	const char* c_write_file = "C:\\Users\\xrd\\Desktop\\openex\\bin\\c_write_file.txt";
	const char* c_write_file_b = "C:\\Users\\xrd\\Desktop\\openex\\bin\\c_write_file_b.txt";
	const char* c_write_file_ab = "C:\\Users\\xrd\\Desktop\\openex\\bin\\c_write_file_ab.txt";
	const char* c_write_file_abp = "C:\\Users\\xrd\\Desktop\\openex\\bin\\c_write_file_abp.txt";

	std::string log_info = "Welcome to OpenEx\n";
	size_t log_length = log_info.length();

	time_t start, end;
	double used_time = 0.0;
	int32_t output_count = 0;
	
	//c++ style writing file  
	std::ofstream of_1( c_plus_write_file_1 );
	assert( of_1 );
	start = clock();
	for( size_t i = 0; i < log_test_times; ++i ) {
		of_1 << log_info;
		of_1 << std::flush;
	}
	end = clock();
	of_1.close();
	used_time = double( end - start ) / 1000.0;
	output_count = int32_t( log_test_times / used_time );
	std::cout << "C++ style 1 耗时：" << used_time << " 秒，";
	std::cout << "平均：" << output_count << " 条/秒。" << "\n" << "\n";

	//c++ style writing file  
	std::ofstream of( c_plus_write_file_2 );
	assert( of );
	start = clock();
	for( size_t i = 0; i < log_test_times; ++i ) {
		of.write( log_info.c_str(), log_length );
		of.flush();
	}
	end = clock();
	of.close();
	used_time = double( end - start ) / 1000.0;
	output_count = int32_t( log_test_times / used_time );
	std::cout << "C++ style 2 耗时：" << used_time << " 秒，";
	std::cout << "平均：" << output_count << " 条/秒。" << "\n" << "\n";

	//c style writing file
	FILE* fp;
	fopen_s( &fp, c_write_file, "w" );
	start = clock();
	for( size_t i = 0; i < log_test_times; ++i ) {
		fwrite( log_info.c_str(), log_length, 1, fp );
		fflush( fp );
	}
	end = clock();
	fclose( fp );
	used_time = double( end - start ) / 1000.0;
	output_count = int32_t( log_test_times / used_time );
	std::cout << "C style 耗时：" << used_time << " 秒，";
	std::cout << "平均：" << output_count << " 条/秒。" << "\n" << "\n";

	//c style writing file binary
	FILE* fp_b;
	fopen_s( &fp_b, c_write_file_b, "wb" );
	start = clock();
	for( size_t i = 0; i < log_test_times; ++i ) {
		fwrite( log_info.c_str(), log_length, 1, fp_b );
		fflush( fp_b );
	}
	end = clock();
	fclose( fp_b );
	used_time = double( end - start ) / 1000.0;
	output_count = int32_t( log_test_times / used_time );
	std::cout << "C style b 耗时：" << used_time << " 秒，";
	std::cout << "平均：" << output_count << " 条/秒。" << "\n" << "\n";

	//c style writing file add binary
	FILE* fp_ab;
	fopen_s( &fp_ab, c_write_file_ab, "ab" );
	start = clock();
	for( size_t i = 0; i < log_test_times; ++i ) {
		fwrite( log_info.c_str(), log_length, 1, fp_ab );
		fflush( fp_ab );
	}
	end = clock();
	fclose( fp_ab );
	used_time = double( end - start ) / 1000.0;
	output_count = int32_t( log_test_times / used_time );
	std::cout << "C style ab 耗时：" << used_time << " 秒，";
	std::cout << "平均：" << output_count << " 条/秒。" << "\n" << "\n";

	//c style writing file add binary
	FILE* fp_abp;
	fopen_s( &fp_abp, c_write_file_abp, "ab+" );
	start = clock();
	for( size_t i = 0; i < log_test_times; ++i ) {
		fwrite( log_info.c_str(), log_length, 1, fp_abp );
		fflush( fp_abp );
	}
	end = clock();
	fclose( fp_abp );
	used_time = double( end - start ) / 1000.0;
	output_count = int32_t( log_test_times / used_time );
	std::cout << "C style abp 耗时：" << used_time << " 秒，";
	std::cout << "平均：" << output_count << " 条/秒。" << "\n" << "\n";

	//std::cin.get();
}

int main( int argc, char *argv[] ) {
	test_write();

	SysLog* sys_log = SysLog::GetInstance();
	sys_log->InitSysLog( "OpenEx", "V0.1.0-Beta Build 20180115" ); // 必须

	std::string log_kind = "norm";
	std::string log_cate = "<SYSTEM_START>";
	std::string log_info = "Welcome to OpenEx";

	sys_log->LogPrint( "debug", 0, "<SYSTEM_START>", log_info );
	sys_log->LogPrint( "norm", 1, "<SYSTEM_START>", log_info );
	sys_log->LogPrint( "prom", 2, "<SYSTEM_START>", log_info );
	sys_log->LogPrint( "warn", 3, "<SYSTEM_START>", log_info );
	sys_log->LogPrint( "error", 4, "<SYSTEM_START>", log_info );
	sys_log->LogPrint( "death", 5, "<SYSTEM_START>", log_info );

	//sys_log->LogPrint( "norm", 1, "<SYSTEM_START>", log_info );
	//sys_log->ClearScreen( 6, 0, true, 1000 );

	size_t log_test_times = 500000;

	time_t start = clock();

	for( size_t i = 0; i < log_test_times; ++i ) {
		sys_log->LogWrite( log_kind, 1, log_cate, log_info );
	}

	time_t end = clock();

	double used_time = double( end - start ) / 1000.0;
	int32_t output_count = int32_t( log_test_times / used_time );
	std::cout << "耗时：" << used_time << " 秒，";
	std::cout << "平均：" << output_count << " 条/秒。" << "\n";

	system( "pause" );

	return 0;
}
