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

#ifndef BASICX_SYSLOG_TESTER_H
#define BASICX_SYSLOG_TESTER_H

#include <vector>

#include <common/define.h>
#include <common/assist.h>

#include "syslog.h"

namespace basicx {

	void Test_SysLog() {
		std::string log_cate = "<SYSLOG_TEST>";
		std::string log_info = "Welcome to SysLog";

		SysLog_S g_syslog( "SysLog_Test" ); // 唯一实例
		SysLog_S* syslog_s = SysLog_S::GetInstance();
		//syslog_s->SetThreadSafe( false );
		//syslog_s->SetLocalCache( true );
		//syslog_s->SetActiveFlush( false );
		//syslog_s->SetActiveSync( false );
		//syslog_s->SetWorkThreads( 2 );
		//syslog_s->SetFileStreamBuffer( DEF_SYSLOG_FSBM_LINE, 4096 ); // 静态链接 MySQL、MariaDB 时需要设为 无缓冲 不然写入文件的日志会被缓存
		syslog_s->InitSysLog( DEF_APP_NAME, DEF_APP_VERSION, DEF_APP_COMPANY, DEF_APP_COPYRIGHT ); // 设置好参数后
		syslog_s->PrintSysInfo();
		syslog_s->LogPrint( syslog_level::n_debug, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::c_debug, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::n_info, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::c_info, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::n_hint, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::c_hint, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::n_warn, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::c_warn, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::n_error, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::c_error, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::n_fatal, log_cate, log_info );
		syslog_s->LogPrint( syslog_level::c_fatal, log_cate, log_info );
		//syslog_s->ClearScreen( 0, 0, false, 2500 );

		SysLog_D* syslog_d = new SysLog_D( "SysLog" );
		syslog_d->SetThreadSafe( false );
		syslog_d->SetLocalCache( true );
		syslog_d->SetActiveFlush( false );
		syslog_d->SetActiveSync( false );
		syslog_d->SetWorkThreads( 2 );
		//syslog_d->SetFileStreamBuffer( DEF_SYSLOG_FSBM_LINE, 4096 ); // 静态链接 MySQL、MariaDB 时需要设为 无缓冲 不然写入文件的日志会被缓存
		syslog_d->InitSysLog( DEF_APP_NAME, DEF_APP_VERSION, DEF_APP_COMPANY, DEF_APP_COPYRIGHT ); // 设置好参数后
		syslog_d->WriteSysInfo();
		syslog_d->LogWrite( syslog_level::n_debug, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::n_info, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::n_hint, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::n_warn, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::n_error, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::n_fatal, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::c_debug, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::c_info, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::c_hint, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::c_warn, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::c_error, log_cate, log_info );
		syslog_d->LogWrite( syslog_level::c_fatal, log_cate, log_info );

		size_t log_test_times = 500000;
		std::vector<std::string> vec_log_infos;
		for( size_t i = 0; i < log_test_times; ++i ) {
			vec_log_infos.push_back( log_info );
		}

		int64_t frequency_log = GetPerformanceFrequency();
		int64_t tick_count_start_log = GetPerformanceTickCount();

		for( size_t i = 0; i < log_test_times; ++i ) {
			syslog_d->LogWrite( syslog_level::n_info, log_cate, vec_log_infos[i], true );
		}

		int64_t tick_count_stop_log = GetPerformanceTickCount();

		// 默认文件缓冲方式和大小 且 SetActiveSync( false ) 且 动态链接：

		// 单个线程，逐条写入：  38.8255 万条/秒
		// 线程安全，逐条写入：  37.7856 万条/秒

		// 单个线程，系统缓存： 123.3499 万条/秒
		// 线程安全，系统缓存： 118.6223 万条/秒

		// 单个线程，本地缓存：1113.9005 万条/秒 // 静态链接只能达到700多万条每秒

		double used_time = double( tick_count_stop_log - tick_count_start_log ) / frequency_log;
		int32_t output_count = int32_t( log_test_times / used_time );
		std::cout << "耗时：" << used_time << " 秒，";
		std::cout << "平均：" << output_count << " 条/秒。" << "\n";

		delete syslog_d;

		Sleep( 10000 );
	}

} // namespace basicx

#endif // BASICX_SYSLOG_TESTER_H
