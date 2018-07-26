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

#ifndef PLUGIN_TEST_1_PLUGIN_TEST_1_P_H
#define PLUGIN_TEST_1_PLUGIN_TEST_1_P_H

#include <syslog/syslog.h>

class Plugin_Test_1_P
{
public:
	Plugin_Test_1_P();
	~Plugin_Test_1_P();

public:
	void SetGlobalPath();

public:
	bool Initialize();
	bool InitializeExt();
	bool StartPlugin();
	bool IsPluginRun();
	bool StopPlugin();
	bool UninitializeExt();
	bool Uninitialize();
	bool AssignTask( int32_t task_id, int32_t identity, int32_t code, std::string& data );

public:
	std::string m_location;
	std::string m_cfg_file_path;
	std::string m_info_file_path;

private:
	std::string m_log_cate;
	basicx::SysLog_S* m_syslog;

// 自定义成员函数和变量

};

#endif // PLUGIN_TEST_1_PLUGIN_TEST_1_P_H
