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

#ifndef BASICX_NETWORK_SERVER_P_H
#define BASICX_NETWORK_SERVER_P_H

#include "struct.h"
#include "server.h"

namespace basicx {

	class SysLog_S;

	class NetServer_P
	{
	public:
		NetServer_P();
		~NetServer_P();

		void ComponentInstance( NetServer_X* net_server_x );

	public:
		void LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show = 0 );

	public:
		void Thread_ServerOnTime();
		void Thread_NetServer();
		void StartNetwork( NetServerCfg& config );
		bool IsNetworkStarted();
		bool IsConnectAvailable( ConnectInfo* connect_info );

		int32_t Server_GetIdentity();

		bool Server_CanAddConnect();
		bool Server_CanAddListen( std::string address_l, int32_t port_l ); // 0.0.0.0
		bool Server_AddListen( std::string address_l, int32_t port_l, std::string node_type_l ); // 0.0.0.0
		void Server_HandleAccept( const boost::system::error_code& error, ConnectInfo* connect_info, AcceptorPtr acceptor );
		void Server_KeepOnAccept( std::string node_type_l, AcceptorPtr acceptor );

		void Server_HandleRecvHead( const boost::system::error_code& error, ConnectInfo* connect_info );
		void Server_HandleRecvData( const boost::system::error_code& error, ConnectInfo* connect_info, int32_t type, int32_t code, int32_t size );

		int32_t Server_SendDataAll( int32_t type, int32_t code, std::string& data );
		int32_t Server_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data );
		void Server_HandleSendMsgs();
		void Server_HandleSendData( const boost::system::error_code& error, SendBufInfo* send_buf_info );

		void Server_CloseOnError( ConnectInfo* connect_info );
		void Server_PassiveClose( ConnectInfo* connect_info );
		void Server_ActiveClose( ConnectInfo* connect_info );
		void Server_CloseAll();
		void Server_Close( ConnectInfo* connect_info );
		void Server_Close( int32_t identity );

		void SendHeartCheck();

		size_t Server_GetConnectCount();
		ConnectInfo* Server_GetConnect( int32_t identity );

	public: // 以下部分容器元素必须使用指针，使用对象会在 push_back 时报 "CObject::CObject 无法访问 private 成员" 的错误
		ServicePtr m_service;
		std::vector<ThreadPtr> m_vec_thread;
		bool m_network_running;
		int32_t m_identity_count;

		TCPSocketOption m_tcp_socket_option;

		std::thread m_sender_thread;
		std::atomic<bool> m_sender_running;
		std::mutex m_sender_lock;
		std::unique_lock<std::mutex> m_unique_lock;
		std::condition_variable m_sender_condition;
		SenderVector* m_sender_vector_1;
		SenderVector* m_sender_vector_2;
		SenderVector* m_sender_vector_read;
		SenderVector* m_sender_vector_write;
		std::mutex m_writing_vector_lock;
		std::mutex m_changing_vector_lock;

		std::mutex m_local_info_lock;
		std::list<ConnectInfo*> m_list_local_info;
		std::mutex m_local_info_index_lock;
		std::unordered_map<int32_t, ConnectInfo*> m_map_local_info_index; // 本地连接索引

		std::mutex m_disconnect_info_lock;
		std::list<ConnectInfo*> m_list_disconnect_info;

		std::mutex m_listen_info_lock;
		std::map<std::string, ListenInfo*> m_map_listen_info;

		int32_t m_log_test;
		int32_t m_heart_check_time;
		size_t m_max_msg_cache_number;
		int32_t m_io_work_thread_number;
		int32_t m_client_connect_timeout; // 毫秒
		// 服务端参数
		size_t m_max_connect_total_s;
		size_t m_max_data_length_s;

		std::thread m_thread_net_server;
		std::thread m_thread_server_on_time;
		size_t m_total_local_connect; // 本地客户端连接数

	private:
		SysLog_S* m_syslog;
		std::string m_log_cate;
		NetServer_X* m_net_server_x;
	};

} // namespace basicx

#endif // BASICX_NETWORK_SERVER_P_H
