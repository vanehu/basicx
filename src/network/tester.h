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

#ifndef NETWORK_TESTER_H
#define NETWORK_TESTER_H

#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>

#include <common/define.h>
#include <syslog/syslog.h>

#include "client.h"
#include "server.h"

namespace basicx {

	// Intel Core i7-4790 3.60GHz - 4.00GHz
	// "Ping-Pong"模式，可以达到 6万次/秒，两个核心跑满
	// 单边模式，两个客户端发给一个服务端，可以达到 20万次/秒，八个核心几乎都满了
	// 开两个或以上 io_work 线程对性能提升不明显
	// 沪深两个交易所，一般一天三千万笔，4 * 3600 秒，大概 2100笔/秒，性能是足够了
	// 改进发送队列以后，"Ping-Pong"模式，可以达到 6.5万次/秒，单边模式 1C 发给 1S 可以达到 26.6万次/秒，2C 发给 1S 因为处理器占用导致性能降低

	class NetworkTester : public NetClient_X, public NetServer_X
	{
	private:
		NetworkTester() {};

	public:
		NetworkTester( std::string pattern )
			: m_pattern( pattern )
			, m_client_connect_info( nullptr )
			, m_server_connect_info( nullptr )
			, m_log_cate( "<NETWORK_TEST>" )
			, m_test_text( "我爱我家，啦啦啦！ABCDEFGabcdefg1234567890!@#$%^&*()" )
			, m_client_recv_count( 0 )
			, m_server_recv_count( 0 ) {
			m_syslog = SysLog_S::GetInstance();

			m_net_client_cfg.m_log_test = 0;
			m_net_client_cfg.m_heart_check_time = 10;
			m_net_client_cfg.m_max_msg_cache_number = 8192; // 0 为不限缓存数量
			m_net_client_cfg.m_io_work_thread_number = 1;
			m_net_client_cfg.m_client_connect_timeout = 2000;
			m_net_client_cfg.m_max_connect_total_c = 1000;
			m_net_client_cfg.m_max_data_length_c = 102400;

			m_net_server_cfg.m_log_test = 0;
			m_net_server_cfg.m_heart_check_time = 10;
			m_net_server_cfg.m_max_msg_cache_number = 8192; // 0 为不限缓存数量
			m_net_server_cfg.m_io_work_thread_number = 1;
			m_net_server_cfg.m_client_connect_timeout = 2000;
			m_net_server_cfg.m_max_connect_total_s = 1000;
			m_net_server_cfg.m_max_data_length_s = 102400;

			m_client.ComponentInstance( this );
			//m_client->Client_SetAutoReconnect( true );
			m_server.ComponentInstance( this );
		};

		~NetworkTester() {};

	public:
		void OnNetClientInfo( NetClientInfo& net_client_info ) {
			std::string log_info;

			switch( net_client_info.m_info_type ) {
			case WM_MY_NEWCONNECT_REMOTE: {
				m_client_connect_info = m_client.Client_GetConnect( net_client_info.m_identity );
				log_info = "客户端 连接 建立完成。";
				m_syslog->LogPrint( syslog_level::c_info, m_log_cate, log_info );
				//m_client.Client_SendData( m_client_connect_info, NW_MSG_TYPE_USER_DATA, NW_MSG_CODE_STRING, m_test_text );
				break;
			}
			case WM_MY_DISCONNECT_REMOTE: {
				m_client_connect_info = nullptr;
				log_info = "客户端 连接 被动断开！";
				m_syslog->LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				break;
			}
			case WM_MY_CLOSECONNECT_CLIENT: {
				m_client_connect_info = nullptr;
				log_info = "客户端 连接 主动断开！";
				m_syslog->LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				break;
			}
			}
		};

		void OnNetClientData( NetClientData& net_client_data ) {
			try {
				if( NW_MSG_CODE_STRING == net_client_data.m_code ) {
					m_client_recv_count++;
					//m_client.Client_SendData( m_client_connect_info, NW_MSG_TYPE_USER_DATA, NW_MSG_CODE_STRING, m_test_text );
					//std::cout << net_client_data.m_data << "\n";
				}
			}
			catch( ... ) {
				std::string log_info = "客户端 接收网络数据 发生未知错误！";
				m_syslog->LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		};

	public:
		void OnNetServerInfo( NetServerInfo& net_server_info ) {
			std::string log_info;

			switch( net_server_info.m_info_type ) {
			case WM_MY_NEWCONNECT_LOCAL: {
				m_server_connect_info = m_server.Server_GetConnect( net_server_info.m_identity );
				log_info = "服务端 连接 建立完成。";
				m_syslog->LogPrint( syslog_level::c_info, m_log_cate, log_info );
				//m_server.Server_SendData( m_server_connect_info, NW_MSG_TYPE_USER_DATA, NW_MSG_CODE_STRING, m_test_text );
				break;
			}
			case WM_MY_DISCONNECT_LOCAL: {
				m_server_connect_info = nullptr;
				log_info = "服务端 连接 被动断开！";
				m_syslog->LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				break;
			}
			case WM_MY_CLOSECONNECT_SERVER: {
				m_server_connect_info = nullptr;
				log_info = "服务端 连接 主动断开！";
				m_syslog->LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				break;
			}
			}
		};

		void OnNetServerData( NetServerData& net_server_data ) {
			try {
				if( NW_MSG_CODE_STRING == net_server_data.m_code ) {
					m_server_recv_count++;
					//m_server.Server_SendData( m_server_connect_info, NW_MSG_TYPE_USER_DATA, NW_MSG_CODE_STRING, m_test_text );
					//std::cout << net_server_data.m_data << "\n";
				}
			}
			catch( ... ) {
				std::string log_info = "服务端 接收网络数据 发生未知错误！";
				m_syslog->LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		};

	public:
		void StartNetwork() {
			if( "c" == m_pattern ) {
				std::cout << "客户端：\n";

				m_client.StartNetwork( m_net_client_cfg );

				if( nullptr != m_client_connect_info ) {
					m_client.Client_Close( m_client_connect_info );
				}

				bool ret = m_client.Client_AddConnect( "127.0.0.1", 2018, "test" );
				if( false == ret ) {
					std::string log_info = "客户端 网络连接 失败！";
					m_syslog->LogPrint( syslog_level::c_error, m_log_cate, log_info );
					return;
				}
				int32_t wait_count = 0;
				while( nullptr == m_client_connect_info && wait_count < 5 ) {
					std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
					wait_count++;
				}
				if( nullptr == m_client_connect_info )
				{
					std::string log_info = "客户端 网络连接 超时！";
					m_syslog->LogPrint( syslog_level::c_error, m_log_cate, log_info );
					return;
				}
			}
			if( "s" == m_pattern ) {
				std::cout << "服务端：\n";
				m_server.StartNetwork( m_net_server_cfg );
				m_server.Server_AddListen( "0.0.0.0", 2018, "test" );
				std::string log_info = "服务端 网络监听 建立。";
				m_syslog->LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		};

		void PrintCount() {
			if( "c" == m_pattern ) {
				std::cout << m_client_recv_count << "\n";
				m_client_recv_count = 0;
				return;
			}
			if( "s" == m_pattern ) {
				std::cout << m_server_recv_count << "\n";
				m_server_recv_count = 0;
				return;
			}
		}

		void ClientSendData() {
			m_client.Client_SendData( m_client_connect_info, NW_MSG_TYPE_USER_DATA, NW_MSG_CODE_STRING, m_test_text );
		}

		void ServerSendData() {
			m_server.Server_SendData( m_server_connect_info, NW_MSG_TYPE_USER_DATA, NW_MSG_CODE_STRING, m_test_text );
		}

	public:
		SysLog_S* m_syslog;
		NetClient m_client;
		NetServer m_server;
		std::string m_pattern;
		std::string m_log_cate;
		std::string m_test_text;
		NetClientCfg m_net_client_cfg;
		NetServerCfg m_net_server_cfg;
		ConnectInfo* m_client_connect_info;
		ConnectInfo* m_server_connect_info;
		std::atomic<int64_t> m_client_recv_count;
		std::atomic<int64_t> m_server_recv_count;
	};

	void Test_Network() {
		std::string pattern;
		std::getline( std::cin, pattern );
		NetworkTester network_tester( pattern );
		network_tester.StartNetwork();
		std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();
		while( true ) {
			if( "c" == pattern ) {
				// 只发 30 秒，查验内存持续增长是因为服务端网络数据堆积导致，而不是内存泄漏
				std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();
				if( ( now_time - start_time ) < std::chrono::duration<int32_t, std::ratio<1, 1>>( 30 ) ) {
					network_tester.ClientSendData();
				}
				else {
					std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
					network_tester.PrintCount();
				}
			}
			if( "s" == pattern ) {
				//network_tester.ServerSendData();
				std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
				network_tester.PrintCount();
			}
		}
	}

} // namespace basicx

#endif // NETWORK_TESTER_H
