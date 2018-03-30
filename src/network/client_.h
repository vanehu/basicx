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

#ifndef BASICX_NETWORK_CLIENT_P_H
#define BASICX_NETWORK_CLIENT_P_H

#include "struct.h"
#include "client.h"

namespace basicx {

	class SysLog_S;

	class NetClient_P
	{
	public:
		NetClient_P();
		~NetClient_P();

		void ComponentInstance( NetClient_X* net_client_x );

	public:
		void LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show = 0 );

	public:
		void Thread_ClientOnTime();
		void Thread_NetClient();
		void StartNetwork( NetClientCfg& config );
		bool IsNetworkStarted();
		bool IsConnectAvailable( ConnectInfo* connect_info );

		int32_t Client_GetIdentity();

		bool Client_CanAddConnect();
		bool Client_CanAddServer( std::string address_r, int32_t port_r ); // 只在限制远程一个地址一个端口只一个连接时检测
		bool Client_AddConnect( std::string address_r, int32_t port_r, std::string node_type_r );
		void Client_HandleConnect( const boost::system::error_code& error, ConnectInfo* connect_info );
		void Client_CheckConnectTime( ConnectInfo* connect_info );
		void Client_SetAutoReconnect( bool auto_reconnect );

		void Client_HandleRecvHead( const boost::system::error_code& error, ConnectInfo* connect_info );
		void Client_HandleRecvData( const boost::system::error_code& error, ConnectInfo* connect_info, int32_t type, int32_t code, int32_t size );
	
		int32_t Client_SendDataAll( int32_t type, int32_t code, std::string& data );
		int32_t Client_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data );
		void Client_HandleSendMsgs();
		void Client_HandleSendData( const boost::system::error_code& error, SendBufInfo* send_buf_info );

		void Client_CloseOnError( ConnectInfo* connect_info );
		void Client_PassiveClose( ConnectInfo* connect_info );
		void Client_ActiveClose( ConnectInfo* connect_info );
		void Client_CloseAll();
		void Client_Close( ConnectInfo* connect_info );
		void Client_Close( int32_t identity );

		void SendHeartCheck();
		void MakeReconnect();

		size_t Client_GetConnectCount();
		ConnectInfo* Client_GetConnect( int32_t identity );

	public: // 以下部分容器元素必须使用指针，使用对象会在 push_back 时报 "CObject::CObject 无法访问 private 成员" 的错误
		ServicePtr m_service;
		std::vector<ThreadPtr> m_vec_thread;
		bool m_network_running;
		int32_t m_identity_count;

		bool m_auto_reconnect_client; // 客户端自动重连 // 服务端没有

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

		std::mutex m_remote_info_lock;
		std::list<ConnectInfo*> m_list_remote_info;
		std::mutex m_remote_info_index_lock;
		std::unordered_map<int32_t, ConnectInfo*> m_map_remote_info_index; // 远程连接索引

		std::mutex m_disconnect_info_lock;
		std::list<ConnectInfo*> m_list_disconnect_info;

		std::mutex m_reconnect_info_lock;
		std::list<ReconnectInfo> m_list_reconnect_info;

		std::mutex m_server_info_lock;
		std::map<std::string, ServerInfo*> m_map_server_info;

		int32_t m_log_test;
		int32_t m_heart_check_time;
		size_t m_max_msg_cache_number;
		int32_t m_io_work_thread_number;
		int32_t m_client_connect_timeout; // 毫秒
		// 客户端参数
		size_t m_max_connect_total_c;
		size_t m_max_data_length_c;

		std::thread m_thread_net_client;
		std::thread m_thread_client_on_time;
		size_t m_total_remote_connect; // 远程服务端连接数

	private:
		SysLog_S* m_syslog;
		std::string m_log_cate;
		NetClient_X* m_net_client_x;
	};

} // namespace basicx

#endif // BASICX_NETWORK_CLIENT_P_H
