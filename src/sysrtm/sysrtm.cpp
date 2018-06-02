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

#include <common/define.h>
#include <common/JsonCpp/json.h>

#include "sysrtm_.h"

namespace basicx {

	SysRtm_P::SysRtm_P() : NetServer_X() {
		m_syscfg = SysCfg_S::GetInstance();
		m_syslog = SysLog_S::GetInstance();
	}

	SysRtm_P::~SysRtm_P() {
	}

	void SysRtm_P::OnNetServerInfo( NetServerInfo& net_server_info ) {
	}

	void SysRtm_P::OnNetServerData( NetServerData& net_server_data ) {
	}

	void SysRtm_P::StartNetServer() {
		CfgBasic* cfg_basic = m_syscfg->GetCfgBasic();

		m_net_server_broad = std::make_shared<NetServer>();
		m_net_server_broad->ComponentInstance( this );
		NetServerCfg net_server_cfg;
		net_server_cfg.m_log_test = cfg_basic->m_debug_infos_server;
		net_server_cfg.m_heart_check_time = cfg_basic->m_heart_check_server;
		net_server_cfg.m_max_msg_cache_number = cfg_basic->m_max_msg_cache_server;
		net_server_cfg.m_io_work_thread_number = cfg_basic->m_work_thread_server;
		net_server_cfg.m_client_connect_timeout = cfg_basic->m_con_time_out_server;
		net_server_cfg.m_max_connect_total_s = cfg_basic->m_con_max_server_server;
		net_server_cfg.m_max_data_length_s = cfg_basic->m_data_length_server;
		m_net_server_broad->StartNetwork( net_server_cfg );

		for( size_t i = 0; i < cfg_basic->m_vec_server_server.size(); i++ ) {
			if( "system_rtm" == cfg_basic->m_vec_server_server[i].m_type && 1 == cfg_basic->m_vec_server_server[i].m_work ) {
				m_net_server_broad->Server_AddListen( "0.0.0.0", cfg_basic->m_vec_server_server[i].m_port, cfg_basic->m_vec_server_server[i].m_type ); // 0.0.0.0
			}
		}
	}

	void SysRtm_P::LogTrans( syslog_level log_level, std::string& log_cate, std::string& log_info ) {
		if( m_net_server_broad->Server_GetConnectCount() > 0 ) {
			time_t now_time_t;
			time( &now_time_t );
			tm now_time_tm = { 0 };
			localtime_s( &now_time_tm, &now_time_t );
			int32_t now_time = now_time_tm.tm_hour * 10000 + now_time_tm.tm_min * 100 + now_time_tm.tm_sec;

			Json::Value json_log_data;
			json_log_data["rtm_func"] = 900001;
			json_log_data["log_time"] = now_time;
			json_log_data["log_level"] = (const char*)&log_level;
			json_log_data["log_cate"] = log_cate;
			json_log_data["log_info"] = log_info;
			Json::StreamWriterBuilder json_writer;
			std::string log_data = Json::writeString( json_writer, json_log_data );

			m_net_server_broad->Server_SendDataAll( NW_MSG_TYPE_USER_DATA, NW_MSG_CODE_JSON, log_data );
		}
	}

	SysRtm_S* SysRtm_S::m_instance = nullptr;

	SysRtm_S::SysRtm_S()
		: m_sysrtm_p( nullptr ) {
		try {
			m_sysrtm_p = new SysRtm_P();
		}
		catch( ... ) {}
		m_instance = this;
	}

	SysRtm_S::~SysRtm_S() {
		if( m_sysrtm_p != nullptr ) {
			delete m_sysrtm_p;
			m_sysrtm_p = nullptr;
		}
	}

	SysRtm_S* SysRtm_S::GetInstance() {
		return m_instance;
	}

	void SysRtm_S::StartNetServer() {
		m_sysrtm_p->StartNetServer();
	}

	void SysRtm_S::LogTrans( syslog_level log_level, std::string& log_cate, std::string& log_info ) {
		m_sysrtm_p->LogTrans( log_level, log_cate, log_info );
	}

} // namespace basicx
