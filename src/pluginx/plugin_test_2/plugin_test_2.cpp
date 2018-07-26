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

#include "define.h"
#include "plugin_test_2.h"
#include "plugin_test_2_.h"

Plugin_Test_2 g_plugin; // 必须

// 预设 Plugin_Test_2_P 函数实现

Plugin_Test_2_P::Plugin_Test_2_P()
	: m_log_cate( "<PLUGIN_TEST_2>" )
	, m_location( "" )
	, m_cfg_file_path( "" )
	, m_info_file_path( "" ) {
	m_syslog = basicx::SysLog_S::GetInstance();
}

Plugin_Test_2_P::~Plugin_Test_2_P() {
}

void Plugin_Test_2_P::SetGlobalPath() {
	basicx::Plugins* plugins = basicx::Plugins::GetInstance();
	m_location = plugins->GetPluginLocationByName( PLUGIN_NAME );
	m_cfg_file_path = plugins->GetPluginCfgFilePathByName( PLUGIN_NAME );
	m_info_file_path = plugins->GetPluginInfoFilePathByName( PLUGIN_NAME );
	m_syslog->LogPrint( basicx::syslog_level::c_debug, m_log_cate, m_location );
	m_syslog->LogPrint( basicx::syslog_level::c_debug, m_log_cate, m_cfg_file_path );
	m_syslog->LogPrint( basicx::syslog_level::c_debug, m_log_cate, m_info_file_path );
}

bool Plugin_Test_2_P::Initialize() {
	SetGlobalPath();
	std::string log_info = "Initialize";
	m_syslog->LogPrint( basicx::syslog_level::c_hint, m_log_cate, log_info );
	return true;
}

bool Plugin_Test_2_P::InitializeExt() {
	std::string log_info = "InitializeExt";
	m_syslog->LogPrint( basicx::syslog_level::c_hint, m_log_cate, log_info );
	return true;
}

bool Plugin_Test_2_P::StartPlugin() {
	std::string log_info = "StartPlugin";
	m_syslog->LogPrint( basicx::syslog_level::c_info, m_log_cate, log_info );
	return true;
}

bool Plugin_Test_2_P::IsPluginRun() {
	std::string log_info = "IsPluginRun";
	m_syslog->LogPrint( basicx::syslog_level::c_info, m_log_cate, log_info );
	return true;
}

bool Plugin_Test_2_P::StopPlugin() {
	std::string log_info = "StopPlugin";
	m_syslog->LogPrint( basicx::syslog_level::c_info, m_log_cate, log_info );
	return true;
}

bool Plugin_Test_2_P::UninitializeExt() {
	std::string log_info = "UninitializeExt";
	m_syslog->LogPrint( basicx::syslog_level::c_warn, m_log_cate, log_info );
	return true;
}

bool Plugin_Test_2_P::Uninitialize() {
	std::string log_info = "Uninitialize";
	m_syslog->LogPrint( basicx::syslog_level::c_warn, m_log_cate, log_info );
	return true;
}

bool Plugin_Test_2_P::AssignTask( int32_t task_id, int32_t identity, int32_t code, std::string& data ) {
	std::string log_info = "AssignTask";
	m_syslog->LogPrint( basicx::syslog_level::c_info, m_log_cate, log_info );
	return true;
}

// 自定义 Plugin_Test_2_P 函数实现

// 预设 Plugin_Test_2 函数实现

Plugin_Test_2::Plugin_Test_2()
	: basicx::Plugins_X( PLUGIN_NAME )
	, m_plugin_test_2_p( nullptr ) {
	try {
		m_plugin_test_2_p = new Plugin_Test_2_P();
	}
	catch( ... ) {}
}

Plugin_Test_2::~Plugin_Test_2() {
	if( m_plugin_test_2_p != nullptr ) {
		delete m_plugin_test_2_p;
		m_plugin_test_2_p = nullptr;
	}
}

bool Plugin_Test_2::Initialize() {
	return m_plugin_test_2_p->Initialize();
}

bool Plugin_Test_2::InitializeExt() {
	return m_plugin_test_2_p->InitializeExt();
}

bool Plugin_Test_2::StartPlugin() {
	return m_plugin_test_2_p->StartPlugin();
}

bool Plugin_Test_2::IsPluginRun() {
	return m_plugin_test_2_p->IsPluginRun();
}

bool Plugin_Test_2::StopPlugin() {
	return m_plugin_test_2_p->StopPlugin();
}

bool Plugin_Test_2::UninitializeExt() {
	return m_plugin_test_2_p->UninitializeExt();
}

bool Plugin_Test_2::Uninitialize() {
	return m_plugin_test_2_p->Uninitialize();
}

bool Plugin_Test_2::AssignTask( int32_t task_id, int32_t identity, int32_t code, std::string& data ) {
	return m_plugin_test_2_p->AssignTask( task_id, identity, code, data );
}

// 自定义 Plugin_Test_2 函数实现
