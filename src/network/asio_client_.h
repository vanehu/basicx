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

#ifndef BASICX_NETWORK_ASIO_CLIENT_P_H
#define BASICX_NETWORK_ASIO_CLIENT_P_H

#include <condition_variable>

#include "struct.h"
#include "asio_client.h"

namespace basicx {

	#pragma pack( push )
	#pragma pack( 1 )

	struct ConnectAsio
	{
		SocketPtr m_socket;
		std::mutex m_mutex;
		std::condition_variable m_condition;
		std::atomic<bool> m_available; // 标记连接是否成功
		std::atomic<bool> m_active_close; // 标记主动或被动关闭

		std::string m_endpoint_l; // 连接标记如"tcp://192.16.1.23:333"
		std::string m_protocol_r;
		std::string m_adress_r;
		int32_t m_port_r;
		std::string m_str_port_r;

		std::string m_endpoint_r; // 连接标记如"tcp://192.16.1.38:465"
		std::string m_protocol_l;
		std::string m_adress_l;
		int32_t m_port_l;
		std::string m_str_port_l;

		std::string m_node_type; // 本地监听或远程连接节点类型
	};

	#pragma pack( pop )

	class AsioClient_P
	{
	public:
		AsioClient_P();
		~AsioClient_P();

	public:
		void LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show = 0 );

	public:
		void Thread_AsioClient();

		void StartNetwork();
		bool IsNetworkStarted();

		ConnectAsio* Client_AddConnect( std::string address, int32_t port, std::string node_type, int32_t time_out_sec );
		void Client_HandleConnect( const boost::system::error_code& error, ConnectAsio* connect_info );

		int32_t Client_ReadData_Sy( ConnectAsio* connect_info, char* data, int32_t size );
		int32_t Client_SendData_Sy( ConnectAsio* connect_info, const char* data, int32_t size );

		void Client_CloseByUser( ConnectAsio* connect_info );
		void Client_CloseOnError( ConnectAsio* connect_info );

	public:
		ServicePtr m_service;
		bool m_network_running;

		TCPSocketOption m_tcp_socket_option;

		std::mutex m_disconnect_info_lock;
		std::list<ConnectAsio*> m_list_disconnect_info;

		std::thread m_thread_asio_client;

	private:
		SysLog_S* m_syslog;
		std::string m_log_cate;
	};

} // namespace basicx

#endif // BASICX_NETWORK_ASIO_CLIENT_P_H
