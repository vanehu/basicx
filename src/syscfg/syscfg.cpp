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

#include <common/sysdef.h>
#include <common/assist.h>
#include <common/PugiXml/pugixml.hpp>

#ifdef __OS_WINDOWS__
#include <windows.h>
#endif

#include "syscfg_.h"

namespace basicx {

	SysCfg_S* SysCfg_S::m_instance = nullptr;

	SysCfg_P::SysCfg_P()
		: m_name_app_full( "" )
		, m_path_app_exec( "" )
		, m_path_app_folder( "" )
		, m_path_cfg_folder( "" )
		, m_path_plu_folder( "" )
		, m_path_ext_folder( "" )
		, m_path_cfg_basic( "" ) {
	}

	SysCfg_P::~SysCfg_P() {
	}

	SysCfg_S::SysCfg_S()
		: m_syscfg_p( nullptr ) {
		try {
			m_syscfg_p = new SysCfg_P();
		}
		catch( ... ) {}
		m_instance = this;
	}

	SysCfg_S::~SysCfg_S() {
		if( m_syscfg_p != nullptr ) {
			delete m_syscfg_p;
			m_syscfg_p = nullptr;
		}
	}

	SysCfg_S* SysCfg_S::GetInstance() {
		return m_instance;
	}

	void SysCfg_S::SetGlobalPath( std::string basic_file ) {
#ifdef __OS_WINDOWS__
		wchar_t char_path[MAX_PATH] = { 0 };
		GetModuleFileName( NULL, char_path, MAX_PATH );
		std::string app_exec_path = StringToAnsiChar( char_path );
#endif

		size_t slash_index = app_exec_path.rfind( '\\' );
		m_syscfg_p->m_name_app_full = app_exec_path.substr( slash_index + 1 );
		m_syscfg_p->m_path_app_exec = app_exec_path;
		m_syscfg_p->m_path_app_folder = app_exec_path.substr( 0, slash_index );
		m_syscfg_p->m_path_cfg_folder = m_syscfg_p->m_path_app_folder + "\\configs";
		m_syscfg_p->m_path_plu_folder = m_syscfg_p->m_path_app_folder + "\\plugins";
		m_syscfg_p->m_path_ext_folder = m_syscfg_p->m_path_app_folder + "\\extdlls";
		m_syscfg_p->m_path_cfg_basic = m_syscfg_p->m_path_app_folder + "\\configs\\" + basic_file;

#ifdef __OS_WINDOWS__
		wchar_t env_buf[2048] = { 0 };
		unsigned long size = GetEnvironmentVariable( L"path", env_buf, 2048 );
		std::wstring env_paths = std::wstring( env_buf );
		env_paths.append( L";" );
		env_paths.append( StringToWideChar( m_syscfg_p->m_path_ext_folder ) );
		env_paths.append( L";" );
		bool result = SetEnvironmentVariable( L"path", env_paths.c_str() );
#endif
	}

	std::string SysCfg_S::GetName_AppFull() {
		return m_syscfg_p->m_name_app_full;
	}

	std::string SysCfg_S::GetPath_AppExec() {
		return m_syscfg_p->m_path_app_exec;
	}

	std::string SysCfg_S::GetPath_AppFolder() {
		return m_syscfg_p->m_path_app_folder;
	}

	std::string SysCfg_S::GetPath_CfgFolder() {
		return m_syscfg_p->m_path_cfg_folder;
	}

	std::string SysCfg_S::GetPath_PluFolder() {
		return m_syscfg_p->m_path_plu_folder;
	}

	std::string SysCfg_S::GetPath_ExtFolder() {
		return m_syscfg_p->m_path_ext_folder;
	}

	std::string SysCfg_S::GetPath_CfgBasic() {
		return m_syscfg_p->m_path_cfg_basic;
	}

	CfgBasic* SysCfg_S::GetCfgBasic() {
		return &( m_syscfg_p->m_cfg_basic );
	}

	bool SysCfg_S::ReadCfgBasic( std::string file_path ) {
		pugi::xml_document document;
		if( !document.load_file( file_path.c_str() ) ) {
			return false;
		}

		pugi::xml_node config_node = document.document_element();
		if( config_node.empty() ) {
			return false;
		}

		pugi::xml_node sys_base_node = config_node.child( "SysBase" );
		if( sys_base_node ) {
		}

		pugi::xml_node network_node = config_node.child( "Network" );
		if( network_node ) {
			m_syscfg_p->m_cfg_basic.m_property_cs = atoi( network_node.child_value( "PropertyCS" ) );
			m_syscfg_p->m_cfg_basic.m_work_thread = atoi( network_node.child_value( "WorkThread" ) );
			m_syscfg_p->m_cfg_basic.m_data_length = atoi( network_node.child_value( "DataLength" ) );
			m_syscfg_p->m_cfg_basic.m_con_time_out = atoi( network_node.child_value( "ConTimeOut" ) );
			m_syscfg_p->m_cfg_basic.m_heart_check = atoi( network_node.child_value( "HeartCheck" ) );
			m_syscfg_p->m_cfg_basic.m_reconnect_c = atoi( network_node.child_value( "ReconnectC" ) );
			m_syscfg_p->m_cfg_basic.m_debug_infos = atoi( network_node.child_value( "DebugInfos" ) );
			m_syscfg_p->m_cfg_basic.m_statistics = atoi( network_node.child_value( "Statistics" ) );
			m_syscfg_p->m_cfg_basic.m_con_max_server = atoi( network_node.child_value( "ConMaxServer" ) );
			m_syscfg_p->m_cfg_basic.m_con_max_client = atoi( network_node.child_value( "ConMaxClient" ) );
			for( pugi::xml_node network_child_node = network_node.first_child(); !network_child_node.empty(); network_child_node = network_child_node.next_sibling() ) {
				if( "Server" == std::string( network_child_node.name() ) ) {
					CfgServer cfg_server;
					cfg_server.m_work = network_child_node.attribute( "Work" ).as_int();
					cfg_server.m_port = network_child_node.attribute( "Port" ).as_int();
					cfg_server.m_type = network_child_node.attribute( "Type" ).value();
					m_syscfg_p->m_cfg_basic.m_vec_server.push_back( cfg_server );
				}
				if( "Client" == std::string( network_child_node.name() ) ) {
					CfgClient cfg_client;
					cfg_client.m_work = network_child_node.attribute( "Work" ).as_int();
					cfg_client.m_address = network_child_node.attribute( "Addr" ).value();
					cfg_client.m_port = network_child_node.attribute( "Port" ).as_int();
					cfg_client.m_type = network_child_node.attribute( "Type" ).value();
					m_syscfg_p->m_cfg_basic.m_vec_client.push_back( cfg_client );
				}
			}
		}

		pugi::xml_node net_server_node = config_node.child( "NetServer" );
		if( net_server_node ) {
			m_syscfg_p->m_cfg_basic.m_work_thread_server = atoi( net_server_node.child_value( "WorkThread" ) );
			m_syscfg_p->m_cfg_basic.m_data_length_server = atoi( net_server_node.child_value( "DataLength" ) );
			m_syscfg_p->m_cfg_basic.m_con_time_out_server = atoi( net_server_node.child_value( "ConTimeOut" ) );
			m_syscfg_p->m_cfg_basic.m_heart_check_server = atoi( net_server_node.child_value( "HeartCheck" ) );
			m_syscfg_p->m_cfg_basic.m_debug_infos_server = atoi( net_server_node.child_value( "DebugInfos" ) );
			m_syscfg_p->m_cfg_basic.m_max_msg_cache_server = atoi( net_server_node.child_value( "MaxMsgCache" ) );
			m_syscfg_p->m_cfg_basic.m_con_max_server_server = atoi( net_server_node.child_value( "ConMaxServer" ) );
			for( pugi::xml_node net_server_child_node = net_server_node.first_child(); !net_server_child_node.empty(); net_server_child_node = net_server_child_node.next_sibling() ) {
				if( "Server" == std::string( net_server_child_node.name() ) ) {
					CfgServer cfg_server;
					cfg_server.m_work = net_server_child_node.attribute( "Work" ).as_int();
					cfg_server.m_port = net_server_child_node.attribute( "Port" ).as_int();
					cfg_server.m_type = net_server_child_node.attribute( "Type" ).value();
					m_syscfg_p->m_cfg_basic.m_vec_server_server.push_back( cfg_server );
				}
			}
		}

		pugi::xml_node net_client_node = config_node.child( "NetClient" );
		if( net_client_node ) {
			m_syscfg_p->m_cfg_basic.m_work_thread_client = atoi( net_client_node.child_value( "WorkThread" ) );
			m_syscfg_p->m_cfg_basic.m_data_length_client = atoi( net_client_node.child_value( "DataLength" ) );
			m_syscfg_p->m_cfg_basic.m_con_time_out_client = atoi( net_client_node.child_value( "ConTimeOut" ) );
			m_syscfg_p->m_cfg_basic.m_heart_check_client = atoi( net_client_node.child_value( "HeartCheck" ) );
			m_syscfg_p->m_cfg_basic.m_debug_infos_client = atoi( net_client_node.child_value( "DebugInfos" ) );
			m_syscfg_p->m_cfg_basic.m_max_msg_cache_client = atoi( net_client_node.child_value( "MaxMsgCache" ) );
			m_syscfg_p->m_cfg_basic.m_con_max_client_client = atoi( net_client_node.child_value( "ConMaxClient" ) );
			for( pugi::xml_node net_client_child_node = net_client_node.first_child(); !net_client_child_node.empty(); net_client_child_node = net_client_child_node.next_sibling() ) {
				if( "Client" == std::string( net_client_child_node.name() ) ) {
					CfgClient cfg_client;
					cfg_client.m_work = net_client_child_node.attribute( "Work" ).as_int();
					cfg_client.m_address = net_client_child_node.attribute( "Addr" ).value();
					cfg_client.m_port = net_client_child_node.attribute( "Port" ).as_int();
					cfg_client.m_type = net_client_child_node.attribute( "Type" ).value();
					m_syscfg_p->m_cfg_basic.m_vec_client_client.push_back( cfg_client );
				}
			}
		}

		pugi::xml_node plugins_node = config_node.child( "Plugins" );
		if( plugins_node ) {
			for( pugi::xml_node plugins_child_node = plugins_node.first_child(); !plugins_child_node.empty(); plugins_child_node = plugins_child_node.next_sibling() ) {
				if( "Plugin" == std::string( plugins_child_node.name() ) ) {
					CfgPlugin cfg_plugin;
					cfg_plugin.m_work = plugins_child_node.attribute( "Work" ).as_int();
					cfg_plugin.m_name = plugins_child_node.attribute( "Name" ).value();
					m_syscfg_p->m_cfg_basic.m_vec_plugin.push_back( cfg_plugin );
				}
			}
		}

		return true;
	}

	bool SysCfg_S::IsHoliday() {
		bool is_holiday = false;

		time_t now_time_t;
		time( &now_time_t );
		tm now_time_tm = { 0 };
		localtime_s( &now_time_tm, &now_time_t );
		int32_t date = ( now_time_tm.tm_mon + 1 ) * 100 + now_time_tm.tm_mday;

		m_syscfg_p->m_holiday_lock.lock();
		for( size_t i = 0; i < m_syscfg_p->m_vec_holiday.size(); i++ ) {
			if( date == m_syscfg_p->m_vec_holiday[i] ) {
				is_holiday = true;
				break;
			}
		}
		m_syscfg_p->m_holiday_lock.unlock();

		return is_holiday;
	}

	bool SysCfg_S::IsWeekend() {
		bool is_weekend = false;

		time_t now_time_t;
		time( &now_time_t );
		tm now_time_tm = { 0 };
		localtime_s( &now_time_tm, &now_time_t );
		int32_t week_day = now_time_tm.tm_wday;
		if( 6 == week_day || 0 == week_day ) {
			is_weekend = true;
		}

		return is_weekend;
	}

} // namespace basicx
