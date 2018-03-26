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

#include <stdlib.h>

#include <common/define.h>
#include <common/sysdef.h>
#include <common/assist.h>
//#include <common/common.h>
//#include <timers/timers.h>
//#include <syslog/syslog.h>
//#include <syscfg/syscfg.h>
//#include <sysrtm/sysrtm.h>
//#include <sysdbi_m/sysdbi_m.h>
//#include <sysdbi_s/sysdbi_s.h>
//#include <network/client.h>
//#include <network/server.h>
//#include <plugins/plugins.h>

#include <common/tester.h>
#include <timers/tester.h>
#include <syslog/tester.h>
#include <sysdbi_m/tester.h>
#include <sysdbi_s/tester.h>
#include <network/tester.h>
#include <plugins/tester.h>

#include "basicx.h"

#pragma comment( lib, "Version.lib" ) // 用 MariaDB 后，静态编译时需要，不然报：无法解析的外部符号 GetFileVersionInfoSizeA、GetFileVersionInfoA、VerQueryValueA
#pragma comment( lib, "Shlwapi.lib" ) // 用 MariaDB 后，静态编译时需要，不然报：无法解析的外部符号 __imp_PathRemoveFileSpecA

int main( int argc, char* argv[] ) {
	basicx::SetMinimumTimerResolution();

	//basicx::Test_Common();
	//basicx::Test_Timers();
	//basicx::Test_SysLog();

	// for network、plugins、sysdbi_m、sysdbi_s test
	//basicx::SysLog_S g_syslog_s( "BasicX" );
	//basicx::SysLog_S* syslog_s = basicx::SysLog_S::GetInstance();
	//syslog_s->SetThreadSafe( false );
	//syslog_s->SetLocalCache( true );
	//syslog_s->SetActiveFlush( false );
	//syslog_s->SetActiveSync( false );
	//syslog_s->SetWorkThreads( 1 );
	////syslog_s->SetFileStreamBuffer( DEF_SYSLOG_FSBM_NONE ); // 会影响性能，但在静态链接 MySQL、MariaDB 时需要设为 无缓冲 不然写入文件的日志会被缓存
	//syslog_s->InitSysLog( DEF_APP_NAME, DEF_APP_VERSION, DEF_APP_COMPANY, DEF_APP_COPYRIGHT ); // 设置好参数后
	//syslog_s->PrintSysInfo();
	//syslog_s->WriteSysInfo();

	//basicx::SysDBI_M g_sysdbi_m;
	//basicx::Test_SysDBI_M();

	//basicx::SysDBI_S g_sysdbi_s;
	//basicx::Test_SysDBI_S();

	//basicx::Test_Network();

	//basicx::Plugins g_plugins;
	//basicx::Plugins* plugins = basicx::Plugins::GetInstance();
	//plugins->StartPlugins();
	//plugins->LoadAll( ".\\plugins" );
	//basicx::Test_Plugins();

	system( "pause" );

	return 0;
}

namespace basicx {

} // namespace basicx
