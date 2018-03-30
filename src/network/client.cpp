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

#include <chrono>
#include <iomanip> // std::setw
#include <iostream>

#include "client_.h"

namespace basicx {

	NetClientInfo::NetClientInfo( int32_t info_type, std::string& node_type, int32_t identity, std::string& endpoint_l, std::string& endpoint_r )
		: m_info_type( info_type )
		, m_node_type( node_type )
		, m_identity( identity )
		, m_endpoint_l( endpoint_l )
		, m_endpoint_r( endpoint_r ) {
	}

	NetClientData::NetClientData( std::string& node_type, int32_t identity, int32_t code, std::string& data )
		: m_node_type( node_type )
		, m_identity( identity )
		, m_code( code ) {
		m_data = std::move( data ); // 性能与使用 std::string& m_data; 然后 m_data( data ) 一样
	}

	NetClient_X::NetClient_X() {
	}

	NetClient_X::~NetClient_X() {
	}

	NetClient_P::NetClient_P()
		: m_net_client_x( nullptr )
		, m_network_running( false )
		, m_identity_count( 0 )
		, m_auto_reconnect_client( false )
		, m_unique_lock( m_sender_lock )
		, m_sender_vector_1( nullptr )
		, m_sender_vector_2( nullptr )
		, m_sender_vector_read( nullptr )
		, m_sender_vector_write( nullptr )
		, m_sender_running( false )
		, m_log_test( 0 )
		, m_heart_check_time( 10 )
		, m_max_msg_cache_number( 0 ) // 0 为不限缓存数量
		, m_io_work_thread_number( 1 )
		, m_client_connect_timeout( 2000 )
		, m_max_connect_total_c( 100 )
		, m_max_data_length_c( 102400 )
		, m_total_remote_connect( 0 )
		, m_log_cate( "<NET_CLIENT>" ) {
		m_syslog = SysLog_S::GetInstance();
	}

	NetClient_P::~NetClient_P() {
		m_sender_running = false;
		m_sender_condition.notify_all();
		m_sender_thread.join();
		if( m_sender_vector_1 != nullptr ) {
			delete m_sender_vector_1;
		}
		if( m_sender_vector_2 != nullptr ) {
			delete m_sender_vector_2;
		}

		if( true == m_network_running ) {
			m_network_running = false;
			m_service->stop();
		}

		for( auto it_ci = m_list_remote_info.begin(); it_ci != m_list_remote_info.end(); it_ci++ ) {
			if( (*it_ci) != nullptr ) {
				(*it_ci)->clear();
				delete (*it_ci);
				(*it_ci) = nullptr;
			}
		}

		for( auto it_si = m_map_server_info.begin(); it_si != m_map_server_info.end(); it_si++ ) {
			if( it_si->second != nullptr ) {
				delete it_si->second;
				it_si->second = nullptr;
			}
		}

		for( auto it_di = m_list_disconnect_info.begin(); it_di != m_list_disconnect_info.end(); it_di++ ) {
			if( (*it_di) != nullptr ) {
				// (*it_di)->clear(); // 放入时已清理
				delete (*it_di);
				(*it_di) = nullptr;
			}
		}
	}

	void NetClient_P::ComponentInstance( NetClient_X* net_client_x ) {
		m_net_client_x = net_client_x;
	}

	void NetClient_P::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show/* = 0*/ ) {
		m_syslog->LogWrite( log_level, log_cate, log_info );
		m_syslog->LogPrint( log_level, log_cate, "LOG>: " + log_info ); // 控制台
	}

	void NetClient_P::Thread_ClientOnTime() {
		std::string log_info;

		log_info = "客户端 创建网络通信定时线程完成, 开始进行网络通信定时 ...";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		try {
			while( true ) {
				std::this_thread::sleep_for( std::chrono::milliseconds( m_heart_check_time * 1000 ) );
				if( IsNetworkStarted() ) {
					SendHeartCheck(); // 心跳检测
					MakeReconnect(); // 只针对客户端发起的且被动断开的连接
				}
			}
		} // try
		catch( ... ) {
			log_info = "客户端 网络通信定时线程发生未知错误！";
			LogPrint( syslog_level::c_fatal, m_log_cate, log_info );
		}

		log_info = "客户端 网络通信定时线程退出！";
		LogPrint( syslog_level::c_warn, m_log_cate, log_info );
	}

	void NetClient_P::Thread_NetClient() {
		std::string log_info;

		log_info = "客户端 创建网络通信服务线程完成, 开始进行网络通信服务 ...";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		try {
			try {
				m_service = std::make_shared<boost::asio::io_service>();
				boost::asio::io_service::work work( *m_service );

				for( int32_t i = 0; i < m_io_work_thread_number; i++ ) {
					ThreadPtr thread_ptr( new std::thread( boost::bind( &boost::asio::io_service::run, m_service ) ) );
					m_vec_thread.push_back( thread_ptr );
				}

				m_network_running = true;

				for( size_t i = 0; i < m_vec_thread.size(); i++ ) { // 等待所有线程退出
					m_vec_thread[i]->join();
				}
			}
			catch( std::exception& ex ) {
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 网络通信服务 初始化 异常：{0}", ex.what() );
				}
				else {
					log_info = "客户端 网络通信服务 初始化 异常！";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		} // try
		catch( ... ) {
			log_info = "客户端 网络通信服务线程发生未知错误！";
			LogPrint( syslog_level::c_fatal, m_log_cate, log_info );
		}

		if( true == m_network_running ) {
			m_network_running = false;
			m_service->stop();
		}

		log_info = "客户端 网络通信服务线程退出！";
		LogPrint( syslog_level::c_warn, m_log_cate, log_info );
	}

	void NetClient_P::StartNetwork( NetClientCfg& config ) {
		std::string log_info;

		m_log_test = config.m_log_test;
		m_heart_check_time = config.m_heart_check_time;
		m_max_msg_cache_number = config.m_max_msg_cache_number;
		m_io_work_thread_number = config.m_io_work_thread_number;
		m_client_connect_timeout = config.m_client_connect_timeout;
		// 客户端参数
		m_max_connect_total_c = config.m_max_connect_total_c;
		m_max_data_length_c = config.m_max_data_length_c;

		m_thread_net_client = std::thread( &NetClient_P::Thread_NetClient, this );
		log_info = "客户端 启动 网络通信服务 线程。";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		m_thread_client_on_time = std::thread( &NetClient_P::Thread_ClientOnTime, this );
		log_info = "客户端 启动 网络通信定时 线程。";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		// 在有界面模式下，要看下这里 while() 是否会导致 LogPrint() 中的 SendMessage 阻塞，或者标记窗口创建完成前不要发送界面消息
		while( false == m_network_running ) {
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}

		m_sender_vector_1 = new SenderVector( m_max_msg_cache_number );
		m_sender_vector_2 = new SenderVector( m_max_msg_cache_number );
		// 所以实际缓存数量是指定数值的两倍
		m_sender_vector_read = m_sender_vector_1;
		m_sender_vector_write = m_sender_vector_1;
		m_sender_running = true;
		m_sender_thread = std::thread( &NetClient_P::Client_HandleSendMsgs, this );
		log_info = "客户端 启动 网络通信处理 线程。";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );
	}

	bool NetClient_P::IsNetworkStarted() {
		return m_network_running;
	}

	bool NetClient_P::IsConnectAvailable( ConnectInfo* connect_info ) {
		if( connect_info != nullptr ) {
			return connect_info->m_available;
		}
		return false;
	}

	// int32_t->2147483647：(000000001~214748364)*10+1：NetClient：1
	int32_t NetClient_P::Client_GetIdentity() {
		if( 214748364 == m_identity_count ) {
			m_identity_count = 0;
		}
		m_identity_count++; //
		return m_identity_count * 10 + 1; // NetClient：1
	}

	bool NetClient_P::Client_CanAddConnect() {
		return m_total_remote_connect < m_max_connect_total_c;
	}

	bool NetClient_P::Client_CanAddServer( std::string address_r, int32_t port_r ) {
		std::string server_endpoint;
		FormatLibrary::StandardLibrary::FormatTo( server_endpoint, "tcp://{0}:{1}", address_r, port_r );

		bool can_add_connect = true;

		m_server_info_lock.lock();
		auto it_si = m_map_server_info.find( server_endpoint );
		if( it_si != m_map_server_info.end() && it_si->second->m_connect_number > 0 ) { // 已存在连接
			can_add_connect = false;
		}
		m_server_info_lock.unlock();

		return can_add_connect;
	}

	bool NetClient_P::Client_AddConnect( std::string address_r, int32_t port_r, std::string node_type_r ) {
		std::string log_info;

		ConnectInfo* connect_info = nullptr;

		try {
			if( !IsNetworkStarted() ) {
				log_info = "客户端 网络通信服务未启动！";
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				return false;
			}

			if( !Client_CanAddConnect() ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 全局连接 {0}:{1} 已满！拒绝连接 {2}:{3}:{4}", m_total_remote_connect, m_max_connect_total_c, address_r, port_r, node_type_r );
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				return false;
			}

			std::string port_r_temp;
			FormatLibrary::StandardLibrary::FormatTo( port_r_temp, "{0}", port_r );
			std::string address_r_temp = address_r;

			// 转换为IP地址
			boost::asio::ip::tcp::resolver resolver( *m_service );
			boost::asio::ip::tcp::resolver::query query( address_r_temp, port_r_temp );
			boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve( query );

			connect_info = new ConnectInfo(); // 如果连接建立后立即发送消息，可能会找不到该连接，因为这里还没放入队列
			connect_info->m_socket = std::make_shared<boost::asio::ip::tcp::socket>( *m_service );
			connect_info->m_connect_timer = std::make_shared<boost::asio::deadline_timer>( *m_service );
			connect_info->m_available = false;
			connect_info->m_node_type = node_type_r; //

			// 记录远程主机信息
			std::string server_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( server_endpoint, "tcp://{0}:{1}", address_r, port_r );
			m_server_info_lock.lock();
			auto it_si = m_map_server_info.find( server_endpoint );
			if( it_si == m_map_server_info.end() ) {
				ServerInfo* server_info = new ServerInfo();
				server_info->m_address = address_r;
				server_info->m_port = port_r;
				server_info->m_node_type = node_type_r;
				server_info->m_connect_number = 0;
				m_map_server_info.insert( std::pair<std::string, ServerInfo*>( server_endpoint, server_info ) );
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 新增 远程主机：{0}:{1}:{2}", address_r, port_r, node_type_r );
				LogPrint( syslog_level::c_info, m_log_cate, log_info );
			}
			m_server_info_lock.unlock();

			// 异步连接操作
			connect_info->m_connect_timer->expires_from_now( boost::posix_time::milliseconds( m_client_connect_timeout ) );
			boost::asio::async_connect( *(connect_info->m_socket), endpoint_iterator, boost::bind( &NetClient_P::Client_HandleConnect, this, boost::asio::placeholders::error, connect_info ) );
			connect_info->m_connect_timer->async_wait( boost::bind( &NetClient_P::Client_CheckConnectTime, this, connect_info ) );
		}
		catch( std::exception& ex ) { // 如果没有错误，则超时机制已建立，connect_info 由超时函数 Client_CheckConnectTime() 负责清理，否则在此清理
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 增加 连接 异常：{0}", ex.what() );
			}
			else {
				log_info = "客户端 增加 连接 异常！";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );

			if( connect_info ) {
				if( connect_info->m_socket.get() ) {
					connect_info->m_socket->close();
				}

				connect_info->m_connect_timer->cancel();

				// TODO：可以加入到重连队列

				delete connect_info;
				connect_info = nullptr;
			}

			return false;
		}

		return true;
	}

	void NetClient_P::Client_CheckConnectTime( ConnectInfo* connect_info ) {
		std::string log_info;

		if( connect_info->m_connect_timer->expires_at() <= boost::asio::deadline_timer::traits_type::now() ) { // 现在时间 >= 超时时间，查看连接是否成功
			if( false == connect_info->m_available ) { // 连接仍未建立
				if( connect_info->m_socket.get() ) {
					connect_info->m_socket->close();
				}

				// TODO：可以加入到重连队列

				delete connect_info;
				connect_info = nullptr;

				log_info = "客户端 增加 连接 超时！";
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
			else { // 连接已建立
				connect_info->m_connect_timer->cancel(); // 不需要再等待超时
				// 或设为永不超时
				// connect_info->m_connect_timer->expires_at( boost::posix_time::pos_infin );
				// connect_info->m_connect_timer->async_wait( boost::bind( &NetClient_P::Client_CheckConnectTime, this, connect_info ) );
			}
		}
		else {
			connect_info->m_connect_timer->async_wait( boost::bind( &NetClient_P::Client_CheckConnectTime, this, connect_info ) );
		} // 未到查看时间，继续等待
	}

	void NetClient_P::Client_HandleConnect( const boost::system::error_code& error, ConnectInfo* connect_info ) {
		std::string log_info;

		if( !error ) {
			connect_info->m_identity = Client_GetIdentity();
			connect_info->m_available = true;
			time( &connect_info->m_heart_check_time );
			connect_info->m_active_close = false;

			connect_info->m_protocol_r = "tcp";
			connect_info->m_adress_r = connect_info->m_socket->remote_endpoint().address().to_string().c_str();
			connect_info->m_port_r = connect_info->m_socket->remote_endpoint().port();
			FormatLibrary::StandardLibrary::FormatTo( connect_info->m_str_port_r, "{0}", connect_info->m_port_r );
			connect_info->m_endpoint_r = connect_info->m_protocol_r + "://" + connect_info->m_adress_r + ":" + connect_info->m_str_port_r; // 连接标记如"tcp://192.16.1.23:333"

			connect_info->m_protocol_l = "tcp";
			connect_info->m_adress_l = connect_info->m_socket->local_endpoint().address().to_string().c_str();
			connect_info->m_port_l = connect_info->m_socket->local_endpoint().port();
			FormatLibrary::StandardLibrary::FormatTo( connect_info->m_str_port_l, "{0}", connect_info->m_port_l );
			connect_info->m_endpoint_l = connect_info->m_protocol_l + "://" + connect_info->m_adress_l + ":" + connect_info->m_str_port_l; // 连接标记如"tcp://192.16.1.38:465"

			connect_info->m_recv_buf_head = new char[HEAD_BYTES];
			connect_info->m_recv_buf_data = new char[1024];
			connect_info->m_recv_buf_data_size = 1024;

			connect_info->m_stat_lost_msg = 0;

			m_remote_info_lock.lock();
			m_list_remote_info.push_back( connect_info );
			m_total_remote_connect = m_list_remote_info.size(); //
			m_remote_info_lock.unlock();
			m_remote_info_index_lock.lock();
			m_map_remote_info_index.insert( std::pair<int32_t, ConnectInfo*>( connect_info->m_identity, connect_info ) );
			m_remote_info_index_lock.unlock();

			FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 新增连接：[{0}]->[{1}]：{2}", connect_info->m_endpoint_l, connect_info->m_endpoint_r, connect_info->m_identity );
			LogPrint( syslog_level::c_info, m_log_cate, log_info );

			// 更新远程主机信息
			std::string server_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( server_endpoint, "tcp://{0}:{1}", connect_info->m_adress_r, connect_info->m_port_r );
			m_server_info_lock.lock();
			auto it_si = m_map_server_info.find( server_endpoint );
			if( it_si != m_map_server_info.end() ) {
				it_si->second->m_connect_number++;
			}
			m_server_info_lock.unlock();

			// 因为重连时 Client_AddConnect() 看不出是否连接成功的，还要看 Client_HandleConnect() 是否报错，所以这里再来判断重连是否成功
			m_reconnect_info_lock.lock();
			for( auto it_ri = m_list_reconnect_info.begin(); it_ri != m_list_reconnect_info.end(); it_ri++ ) {
				if( it_ri->m_address == connect_info->m_adress_r && it_ri->m_port == connect_info->m_port_r && it_ri->m_node_type == connect_info->m_node_type ) { // 随便哪一个，只要地址端口类型一致就行
					m_list_reconnect_info.erase( it_ri ); // 说明这个重连已成功
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 重连 {0}:{1}:{2} 成功。", it_ri->m_address, it_ri->m_port, it_ri->m_node_type );
					LogPrint( syslog_level::c_info, m_log_cate, log_info );
					break;
				}
			}
			m_reconnect_info_lock.unlock();

			try {
				connect_info->m_socket->set_option( boost::asio::ip::tcp::no_delay( m_tcp_socket_option.m_no_delay ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::keep_alive( m_tcp_socket_option.m_keep_alive ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::enable_connection_aborted( m_tcp_socket_option.m_enable_connection_aborted ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::receive_buffer_size( m_tcp_socket_option.m_recv_buffer_size ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::send_buffer_size( m_tcp_socket_option.m_send_buffer_size ) );

				// 准备从服务端接收 Type 数据
				memset( connect_info->m_recv_buf_head, 0, HEAD_BYTES );
				boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_head, HEAD_BYTES ), boost::bind( &NetClient_P::Client_HandleRecvHead, this, boost::asio::placeholders::error, connect_info ) );
			}
			catch( std::exception& ex ) {
				Client_CloseOnError( connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 连接 处理 异常：{0}", ex.what() );
				}
				else {
					log_info = "客户端 连接 处理 异常！";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}

			if( nullptr != m_net_client_x ) {
				NetClientInfo net_client_info( WM_MY_NEWCONNECT_REMOTE, connect_info->m_node_type, connect_info->m_identity, connect_info->m_endpoint_l, connect_info->m_endpoint_r ); // 网络通信模块发起新的向服务端连接
				m_net_client_x->OnNetClientInfo( net_client_info );
			}
		}
		else {
			// 收拾工作由超时函数 Client_CheckConnectTime() 来处理
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 连接 处理 错误：{0}", error.message().c_str() );
			}
			else {
				log_info = "客户端 连接 处理 错误！";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	void NetClient_P::Client_SetAutoReconnect( bool auto_reconnect ) {
		m_auto_reconnect_client = auto_reconnect;
	}

	void NetClient_P::Client_HandleRecvHead( const boost::system::error_code& error, ConnectInfo* connect_info ) {
		if( !error ) {
			try {
				int32_t type( 0 );
				int32_t code( 0 );
				int32_t size( 0 );
				std::istringstream type_temp( std::string( connect_info->m_recv_buf_head, TYPE_BYTES ) );
				std::istringstream code_temp( std::string( connect_info->m_recv_buf_head, TYPE_BYTES, CODE_BYTES ) );
				std::istringstream size_temp( std::string( connect_info->m_recv_buf_head, FLAG_BYTES, SIZE_BYTES ) );
				type_temp >> std::hex >> type;
				code_temp >> std::hex >> code;
				size_temp >> std::hex >> size;

				if( 1 == m_log_test ) {
					std::string log_info;
					FormatLibrary::StandardLibrary::FormatTo( log_info, "<--[{0}]：Type:{1} Code:{2} Size:{3}", connect_info->m_endpoint_r, type, code, size );
					LogPrint( syslog_level::c_info, m_log_cate, log_info );
				}

				if( type < NW_MSG_ATOM_TYPE_MIN || type > NW_MSG_USER_TYPE_MAX ) {
					std::string log_info;
					Client_CloseOnError( connect_info );
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 收到 未知的数据包类型：{0}", type );
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
					return;
				}

				if( code < NW_MSG_CODE_TYPE_MIN || code > NW_MSG_CODE_TYPE_MAX ) {
					std::string log_info;
					Client_CloseOnError( connect_info );
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 收到 未知的数据包编码：{0}", code );
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
					return;
				}

				if( 0 == size ) {
					// 准备从服务端接收 Head 数据
					memset( connect_info->m_recv_buf_head, 0, HEAD_BYTES );
					boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_head, HEAD_BYTES ), boost::bind( &NetClient_P::Client_HandleRecvHead, this, boost::asio::placeholders::error, connect_info ) );
				}
				else if( size > 0 && size <= m_max_data_length_c ) {
					// 准备从服务端接收 Data 数据
					if( connect_info->m_recv_buf_data_size < size ) {
						delete[] connect_info->m_recv_buf_data;
						connect_info->m_recv_buf_data = new char[size];
						connect_info->m_recv_buf_data_size = size;
					}
					memset( connect_info->m_recv_buf_data, 0, size );
					boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_data, size ), boost::bind( &NetClient_P::Client_HandleRecvData, this, boost::asio::placeholders::error, connect_info, type, code, size ) );
				}
				else {
					std::string log_info;
					Client_CloseOnError( connect_info );
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 收到 异常的数据包大小：{0}", size );
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Client_CloseOnError( connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 接收 Head 处理 异常：{0}", ex.what() );
				}
				else {
					log_info = "客户端 接收 Head 处理 异常！";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		}
		else {
			std::string log_info;
			Client_CloseOnError( connect_info );
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 接收 Head 处理 错误：{0}", error.message().c_str() );
			}
			else {
				log_info = "客户端 接收 Head 处理 错误！";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	void NetClient_P::Client_HandleRecvData( const boost::system::error_code& error, ConnectInfo* connect_info, int32_t type, int32_t code, int32_t size ) {
		if( !error ) {
			try {
				std::string data( connect_info->m_recv_buf_data, size );

				if( 1 == m_log_test ) {
					std::string log_info;
					FormatLibrary::StandardLibrary::FormatTo( log_info, "<--[{0}]：Data:{1}", connect_info->m_endpoint_r, data.length() );
					LogPrint( syslog_level::c_info, m_log_cate, log_info );
				}

				if( !data.empty() ) {
					try {
						if( m_net_client_x != nullptr ) {
							NetClientData net_client_data( connect_info->m_node_type, connect_info->m_identity, code, data ); // std::move( data )
							m_net_client_x->OnNetClientData( net_client_data );
						}
						else {
							std::string log_info;
							log_info = "网络通信调用模块指针 m_net_client_x 为空！";
							LogPrint( syslog_level::c_warn, m_log_cate, log_info );
						}
					}
					catch( ... ) {
						std::string log_info;
						log_info = "客户端 接收 Data 处理 转发数据 发生未知错误！";
						LogPrint( syslog_level::c_error, m_log_cate, log_info );
					}

					// 准备从服务端接收 Type 数据
					memset( connect_info->m_recv_buf_head, 0, HEAD_BYTES );
					boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_head, HEAD_BYTES ), boost::bind( &NetClient_P::Client_HandleRecvHead, this, boost::asio::placeholders::error, connect_info ) );
				}
				else {
					std::string log_info;
					Client_CloseOnError( connect_info );
					log_info = "客户端 收到 数据包内容为空！";
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Client_CloseOnError( connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 接收 Data 处理 异常：{0}", ex.what() );
				}
				else {
					log_info = "客户端 接收 Data 处理 异常！";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		}
		else {
			std::string log_info;
			Client_CloseOnError( connect_info );
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 接收 Data 处理 错误：{0}", error.message().c_str() );
			}
			else {
				log_info = "客户端 接收 Data 处理 错误！";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	int32_t NetClient_P::Client_SendDataAll( int32_t type, int32_t code, std::string& data ) {
		m_remote_info_lock.lock();
		std::list<ConnectInfo*> list_remote_info = m_list_remote_info;
		m_remote_info_lock.unlock();
		int32_t send_count = 0;
		for( auto it_ci = list_remote_info.begin(); it_ci != list_remote_info.end(); it_ci++ ) {
			if( 0 == Client_SendData( *it_ci, type, code, data ) ) {
				send_count++;
			}
		}
		return send_count;
	}

	int32_t NetClient_P::Client_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data ) {
		int32_t result = 0;
		if( connect_info != nullptr && connect_info->m_available != false ) {
			size_t size = data.length();
			if( size <= m_max_data_length_c ) {
				std::ostringstream type_temp;
				std::ostringstream code_temp;
				std::ostringstream size_temp;
				type_temp << std::setw( TYPE_BYTES ) << std::hex << type;
				code_temp << std::setw( CODE_BYTES ) << std::hex << code;
				size_temp << std::setw( SIZE_BYTES ) << std::hex << size;

				//std::string str_type = type_temp.str();
				//std::string str_code = code_temp.str();
				//std::string str_size = size_temp.str();
				//std::string log_info;
				//FormatLibrary::StandardLibrary::FormatTo( log_info, "AddSend：{0} {1} {2} {3}", str_type.c_str(), str_code.c_str(), str_size.c_str(), data.c_str() );
				//LogPrint( syslog_level::c_info, m_log_cate, log_info );

				m_writing_vector_lock.lock();
				try {
					bool sender_is_full = false;
					if( m_sender_vector_write->m_count == m_sender_vector_write->m_capacity ) { // 需换队列，必需写满 capacity 才考虑更换
						m_changing_vector_lock.lock();
						if( m_sender_vector_write == m_sender_vector_read ) { // 读线程未占用目标队列，允许切换
							m_sender_vector_write = m_sender_vector_write == m_sender_vector_1 ? m_sender_vector_2 : m_sender_vector_1; // 切换队列
							m_sender_vector_write->m_count = 0; // 重新开始所在队列计数
						}
						else {
							if( 0 == m_max_msg_cache_number ) { // 不限缓存数量
								m_sender_vector_write->m_capacity += m_sender_vector_write->m_capacity; // 增加点允许长度，等待读线程离开目标队列
							}
							else { // 限制缓存数量
								sender_is_full = true;
								connect_info->m_stat_lost_msg++;
								result = -1;
							}
						}
						m_changing_vector_lock.unlock();
					}
					if( false == sender_is_full ) {
						if( m_sender_vector_write->m_count < m_sender_vector_write->m_vec_send_buf_info.size() ) { // 使用旧有元素
							m_sender_vector_write->m_vec_send_buf_info[m_sender_vector_write->m_count]->Update( connect_info, size, type_temp, code_temp, size_temp, data );
							m_sender_vector_write->m_count++; // 更新以后
						}
						else { // 使用新建元素
							m_sender_vector_write->m_vec_send_buf_info.push_back( new SendBufInfo( connect_info, size, type_temp, code_temp, size_temp, data ) );
							m_sender_vector_write->m_count++; // 添加以后
						}
						m_sender_condition.notify_all(); //
					}
				}
				catch( std::exception& ex ) {
					std::string log_info;
					if( 1 == m_log_test ) {
						FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 添加 Data 消息 异常：{0}", ex.what() );
					}
					else {
						log_info = "客户端 添加 Data 消息 异常！";
					}
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
					result = -2;
				}
				m_writing_vector_lock.unlock();
			}
			else {
				std::string log_info;
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 添加 Data 消息 长度溢出！{0} > {1}", size, m_max_data_length_c );
				LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				result = -3;
			}
		}
		else {
			//std::string log_info = "客户端 添加 Data 消息 连接已断开！";
			//LogPrint( syslog_level::c_warn, m_log_cate, log_info );
			result = -4;
		}
		return result;
	}

	void NetClient_P::Client_HandleSendMsgs() {
		while( true == m_sender_running ) {
			m_sender_condition.wait( m_unique_lock );
			for( ; m_sender_vector_read->m_handled < m_sender_vector_read->m_count; ) {
				m_sender_vector_read->m_handled++;
				SendBufInfo* send_buf_info = m_sender_vector_read->m_vec_send_buf_info[m_sender_vector_read->m_handled - 1];
				try {
					if( send_buf_info->m_connect_info != nullptr && send_buf_info->m_connect_info->m_available != false ) { // 必须！不然可能连接已断开还会去发送数据，读取端会发现连接已断开并清除缓存的数据
						boost::asio::async_write( *(send_buf_info->m_connect_info->m_socket), boost::asio::buffer( send_buf_info->m_send_buf ), boost::bind( &NetClient_P::Client_HandleSendData, this, boost::asio::placeholders::error, send_buf_info ) );
					}
				}
				catch( std::exception& ex ) {
					std::string log_info;
					Client_CloseOnError( send_buf_info->m_connect_info );
					if( 1 == m_log_test ) {
						FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 处理 Msgs 发送 异常：{0}", ex.what() );
					}
					else {
						log_info = "客户端 处理 Msgs 发送 异常！";
					}
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}
			}
			while( m_sender_vector_read->m_handled == m_sender_vector_read->m_capacity ) { // 需换队列，必需读满 capacity 才考虑更换
				bool changed = false;
				m_changing_vector_lock.lock();
				if( m_sender_vector_read != m_sender_vector_write ) { // 写线程已在目标队列，允许切换
					m_sender_vector_read = m_sender_vector_read == m_sender_vector_1 ? m_sender_vector_2 : m_sender_vector_1; // 切换队列
					m_sender_vector_read->m_handled = 0; // 重新开始所在队列计数
					changed = true; //
				}
				// else {} // 等待写线程跳到目标队列
				m_changing_vector_lock.unlock();
				if( true == changed ) { // 已换到新队列
					for( ; m_sender_vector_read->m_handled < m_sender_vector_read->m_count; ) {
						m_sender_vector_read->m_handled++;
						SendBufInfo* send_buf_info = m_sender_vector_read->m_vec_send_buf_info[m_sender_vector_read->m_handled - 1];
						try {
							if( send_buf_info->m_connect_info != nullptr && send_buf_info->m_connect_info->m_available != false ) { // 必须！不然可能连接已断开还会去发送数据，读取端会发现连接已断开并清除缓存的数据
								boost::asio::async_write( *(send_buf_info->m_connect_info->m_socket), boost::asio::buffer( send_buf_info->m_send_buf ), boost::bind( &NetClient_P::Client_HandleSendData, this, boost::asio::placeholders::error, send_buf_info ) );
							}
						}
						catch( std::exception& ex ) {
							std::string log_info;
							Client_CloseOnError( send_buf_info->m_connect_info );
							if( 1 == m_log_test ) {
								FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 处理 Msgs 发送 异常：{0}", ex.what() );
							}
							else {
								log_info = "客户端 处理 Msgs 发送 异常！";
							}
							LogPrint( syslog_level::c_error, m_log_cate, log_info );
						}
					}
				}
				// 在处理消息过程中，写线程可能又把队列写满了，所以查验是否继续切换
			}
		}
	}

	void NetClient_P::Client_HandleSendData( const boost::system::error_code& error, SendBufInfo* send_buf_info ) {
		if( !error ) {
			try {
				if( 1 == m_log_test ) {
					int32_t type( 0 );
					std::istringstream type_temp( std::string( send_buf_info->m_send_buf, 0, TYPE_BYTES ) );
					type_temp >> std::hex >> type;
					int32_t code( 0 );
					std::istringstream code_temp( std::string( send_buf_info->m_send_buf, TYPE_BYTES, CODE_BYTES ) );
					code_temp >> std::hex >> code;
					int32_t size( 0 );
					std::istringstream size_temp( std::string( send_buf_info->m_send_buf, FLAG_BYTES, SIZE_BYTES ) );
					size_temp >> std::hex >> size;
					std::string data( send_buf_info->m_send_buf, HEAD_BYTES, size );
					std::string log_info;
					FormatLibrary::StandardLibrary::FormatTo( log_info, "-->[{0}]：Type:{1} Code:{2} Size:{3} Data:{4}", send_buf_info->m_connect_info->m_endpoint_r, type, code, size, data.length() );
					LogPrint( syslog_level::c_info, m_log_cate, log_info );
				}
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Client_CloseOnError( send_buf_info->m_connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 发送 Data 处理 异常：{0}", ex.what() );
				}
				else {
					log_info = "客户端 发送 Data 处理 异常！";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		}
		else {
			std::string log_info;
			Client_CloseOnError( send_buf_info->m_connect_info );
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 发送 Data 处理 错误：{0}", error.message().c_str() );
			}
			else {
				log_info = "客户端 发送 Data 处理 错误！";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	//int32_t NetClient_P::Client_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data ) {
	//	int32_t result = 0;

	//	if( connect_info != nullptr && connect_info->m_available != false ) {
	//		try {
	//			size_t size = data.length();
	//			if( size <= m_max_data_length_c ) {
	//				std::ostringstream type_temp;
	//				std::ostringstream code_temp;
	//				std::ostringstream size_temp;
	//				type_temp << std::setw( TYPE_BYTES ) << std::hex << type;
	//				code_temp << std::setw( CODE_BYTES ) << std::hex << code;
	//				size_temp << std::setw( SIZE_BYTES ) << std::hex << size;

	//				//std::string str_type = type_temp.str();
	//				//std::string str_code = code_temp.str();
	//				//std::string str_size = size_temp.str();
	//				//std::string log_info;
	//				//FormatLibrary::StandardLibrary::FormatTo( log_info, "AddSend：{0} {1} {2} {3}", str_type.c_str(), str_code.c_str(), str_size.c_str(), data.c_str() );
	//				//LogPrint( syslog_level::c_info, m_log_cate, log_info );

	//				connect_info->m_send_buf_info_lock.lock();
	//				bool write_in_progress = !connect_info->m_list_send_buf_infos.empty();
	//				if( m_max_msg_cache_number <= 0 || connect_info->m_list_send_buf_infos.size() < m_max_msg_cache_number ) {
	//					SendBufInfo send_buf_info;
	//					connect_info->m_list_send_buf_infos.push_back( send_buf_info );
	//					SendBufInfo& send_buf_info_ref = connect_info->m_list_send_buf_infos.back();
	//					send_buf_info_ref.m_send_size = size;
	//					send_buf_info_ref.m_send_buf.clear();
	//					send_buf_info_ref.m_send_buf.append( type_temp.str() );
	//					send_buf_info_ref.m_send_buf.append( code_temp.str() );
	//					send_buf_info_ref.m_send_buf.append( size_temp.str() );
	//					send_buf_info_ref.m_send_buf.append( data );
	//				}
	//				else {
	//					connect_info->m_stat_lost_msg++;
	//				}
	//				connect_info->m_send_buf_info_lock.unlock();

	//				if( !write_in_progress && true == m_network_running ) {
	//					m_service->post( boost::bind( &NetClient_P::Client_HandleSendMsgs, this, connect_info ) );
	//				}

	//				result = 1;
	//			}
	//			else {
	//				std::string log_info;
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 添加 Data 消息 长度溢出！{0} > {1}", size, m_max_data_length_c );
	//				LogPrint( syslog_level::c_warn, m_log_cate, log_info );
	//				result = -2;
	//			}
	//		}
	//		catch( std::exception& ex ) {
	//			result = -1;
	//			std::string log_info;
	//			if( 1 == m_log_test ) {
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 添加 Data 消息 异常：{0}", ex.what() );
	//			}
	//			else {
	//				log_info = "客户端 添加 Data 消息 异常！";
	//			}
	//			LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//		}
	//	}

	//	return result;
	//}

	//void NetClient_P::Client_HandleSendMsgs( ConnectInfo* connect_info ) {
	//	try {
	//		SendBufInfo* send_buf_info = &connect_info->m_list_send_buf_infos.front(); // 肯定有
	//		if( true == connect_info->m_available ) { // 必须！不然可能连接已断开还会去发送数据，读取端会发现连接已断开并清除缓存的数据
	//			boost::asio::async_write( *(connect_info->m_socket), boost::asio::buffer( send_buf_info->m_send_buf ), boost::bind( &NetClient_P::Client_HandleSendData, this, boost::asio::placeholders::error, connect_info ) );
	//		}
	//	}
	//	catch( std::exception& ex ) {
	//		std::string log_info;
	//		Client_CloseOnError( connect_info );
	//		if( 1 == m_log_test ) {
	//			FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 处理 Msgs 发送 异常：{0}", ex.what() );
	//		}
	//		else {
	//			log_info = "客户端 处理 Msgs 发送 异常！";
	//		}
	//		LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//	}
	//}

	//void NetClient_P::Client_HandleSendData( const boost::system::error_code& error, ConnectInfo* connect_info ) {
	//	if( !error ) {
	//		try {
	//			SendBufInfo* send_buf_info = &connect_info->m_list_send_buf_infos.front(); // 肯定有
	//		
	//			if( 1 == m_log_test ) {
	//				int32_t type( 0 );
	//				std::istringstream type_temp( std::string( send_buf_info->m_send_buf, 0, TYPE_BYTES ) );
	//				type_temp >> std::hex >> type;
	//				int32_t code( 0 );
	//				std::istringstream code_temp( std::string( send_buf_info->m_send_buf, TYPE_BYTES, CODE_BYTES ) );
	//				code_temp >> std::hex >> code;
	//				int32_t size( 0 );
	//				std::istringstream size_temp( std::string( send_buf_info->m_send_buf, FLAG_BYTES, SIZE_BYTES ) );
	//				size_temp >> std::hex >> size;
	//				std::string data( send_buf_info->m_send_buf, HEAD_BYTES, size );
	//				std::string log_info;
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "-->[{0}]：Type:{1} Code:{2} Size:{3} Data:{4}", connect_info->m_endpoint_r, type, code, size, data.length() );
	//				LogPrint( syslog_level::c_info, m_log_cate, log_info );
	//			}

	//			connect_info->m_send_buf_info_lock.lock();
	//			connect_info->m_list_send_buf_infos.pop_front();
	//			bool write_in_progress = !connect_info->m_list_send_buf_infos.empty();
	//			connect_info->m_send_buf_info_lock.unlock();

	//			if( write_in_progress && true == m_network_running ) {
	//				m_service->post( boost::bind( &NetClient_P::Client_HandleSendMsgs, this, connect_info ) );
	//			}
	//		}
	//		catch( std::exception& ex ) {
	//			std::string log_info;
	//			Client_CloseOnError( connect_info );
	//			if( 1 == m_log_test ) {
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 发送 Data 处理 异常：{0}", ex.what() );
	//			}
	//			else {
	//				log_info = "客户端 发送 Data 处理 异常！";
	//			}
	//			LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//		}
	//	}
	//	else {
	//		std::string log_info;
	//		Client_CloseOnError( connect_info );
	//		if( 1 == m_log_test ) {
	//			FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 发送 Data 处理 错误：{0}", error.message().c_str() );
	//		}
	//		else {
	//			log_info = "客户端 发送 Data 处理 错误！";
	//		}
	//		LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//	}
	//}

	void NetClient_P::Client_CloseOnError( ConnectInfo* connect_info ) {
		if( connect_info ) {
			if( true == connect_info->m_active_close ) { // 主动关闭
				Client_ActiveClose( connect_info );
			}
			else { // 被动关闭
				Client_PassiveClose( connect_info );
			}
		}
	}

	void NetClient_P::Client_PassiveClose( ConnectInfo* connect_info ) {
		std::string log_info;

		if( connect_info ) {
			connect_info->m_available = false;

			m_remote_info_lock.lock();
			for( auto it_ci = m_list_remote_info.begin(); it_ci != m_list_remote_info.end(); it_ci++ ) {
				if( (*it_ci)->m_identity == connect_info->m_identity ) {
					m_list_remote_info.erase( it_ci );
					m_total_remote_connect = m_list_remote_info.size(); //
					break;
				}
			}
			m_remote_info_lock.unlock();
			m_remote_info_index_lock.lock();
			m_map_remote_info_index.erase( connect_info->m_identity );
			m_remote_info_index_lock.unlock();

			// 更新远程主机信息
			std::string server_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( server_endpoint, "tcp://{0}:{1}", connect_info->m_adress_r, connect_info->m_port_r );
			m_server_info_lock.lock();
			auto it_si = m_map_server_info.find( server_endpoint );
			if( it_si != m_map_server_info.end() ) {
				it_si->second->m_connect_number--;
			}
			m_server_info_lock.unlock();

			if( connect_info->m_socket.get() ) {
				connect_info->m_socket->close();
			}

			FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 被动 关闭连接。[{0}]->[{1}]", connect_info->m_endpoint_l, connect_info->m_endpoint_r );
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );

			if( nullptr != m_net_client_x ) {
				NetClientInfo net_client_info( WM_MY_DISCONNECT_REMOTE, connect_info->m_node_type, connect_info->m_identity, connect_info->m_endpoint_l, connect_info->m_endpoint_r ); // 网络通信模块收到服务端断开连接
				m_net_client_x->OnNetClientInfo( net_client_info );
			}

			// 客户端连接被动关闭时自动重连
			if( true == m_auto_reconnect_client ) {
				ReconnectInfo reconnect_info;
				reconnect_info.m_identity = connect_info->m_identity;
				reconnect_info.m_address = connect_info->m_adress_r;
				reconnect_info.m_port = connect_info->m_port_r;
				reconnect_info.m_node_type = connect_info->m_node_type;
				reconnect_info.m_reconnect_count = 0;
				bool in_reconnect_list = false;
				m_reconnect_info_lock.lock();
				for( auto it_ri = m_list_reconnect_info.begin(); it_ri != m_list_reconnect_info.end(); it_ri++ ) {
					if( it_ri->m_identity == connect_info->m_identity ) {
						in_reconnect_list = true; // 避免连接断开时可能两次报错产生多个重连对象
						break;
					}
				}
				if( false == in_reconnect_list ) {
					m_list_reconnect_info.push_back( reconnect_info );
				}
				m_reconnect_info_lock.unlock();
				if( false == in_reconnect_list ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 连接 {0}:{1}:{2} 被动关闭，转入重连队列！", connect_info->m_adress_r, connect_info->m_port_r, connect_info->m_node_type );
					LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				}
			}

			// 目前不进行删除
			m_disconnect_info_lock.lock();
			connect_info->clear();
			m_list_disconnect_info.push_back( connect_info );
			m_disconnect_info_lock.unlock();
		}
	}

	void NetClient_P::Client_ActiveClose( ConnectInfo* connect_info ) {
		std::string log_info;

		if( connect_info ) {
			connect_info->m_available = false;

			m_remote_info_lock.lock();
			for( auto it_ci = m_list_remote_info.begin(); it_ci != m_list_remote_info.end(); it_ci++ ) {
				if( (*it_ci)->m_identity == connect_info->m_identity ) {
					m_list_remote_info.erase( it_ci );
					m_total_remote_connect = m_list_remote_info.size(); //
					break;
				}
			}
			m_remote_info_lock.unlock();
			m_remote_info_index_lock.lock();
			m_map_remote_info_index.erase( connect_info->m_identity );
			m_remote_info_index_lock.unlock();

			// 更新远程主机信息
			std::string server_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( server_endpoint, "tcp://{0}:{1}", connect_info->m_adress_r, connect_info->m_port_r );
			m_server_info_lock.lock();
			auto it_si = m_map_server_info.find( server_endpoint );
			if( it_si != m_map_server_info.end() ) {
				it_si->second->m_connect_number--;
			}
			m_server_info_lock.unlock();

			if( connect_info->m_socket.get() ) {
				connect_info->m_socket->close();
			}

			FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 主动 关闭连接。[{0}]->[{1}]", connect_info->m_endpoint_l, connect_info->m_endpoint_r );
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );

			if( nullptr != m_net_client_x ) {
				NetClientInfo net_client_info( WM_MY_CLOSECONNECT_CLIENT, connect_info->m_node_type, connect_info->m_identity, connect_info->m_endpoint_l, connect_info->m_endpoint_r ); // 网络通信模块客户端主动关闭连接
				m_net_client_x->OnNetClientInfo( net_client_info );
			}

			// 客户端连接主动关闭时自动重连 // 目前不允许在主动关闭时自动重连
			//if( true == m_auto_reconnect_client ) {
			//	ReconnectInfo reconnect_info;
			//	reconnect_info.m_identity = connect_info->m_identity;
			//	reconnect_info.m_address = connect_info->m_address_r;
			//	reconnect_info.m_port = connect_info->m_port_r;
			//	reconnect_info.m_node_type = connect_info->m_node_type;
			//	reconnect_info.m_reconnect_count = 0;
			//	bool in_reconnect_list = false;
			//	m_reconnect_info_lock.lock();
			//	for( auto it_ri = m_list_reconnect_info.begin(); it_ri != m_list_reconnect_info.end(); it_ri++ ) {
			//		if( it_ri->m_identity == connect_info->m_identity ) {
			//			in_reconnect_list = true; // 避免连接断开时可能两次报错产生多个重连对象
			//			break;
			//		}
			//	}
			//	if( false == in_reconnect_list ) {
			//		m_list_reconnect_info.push_back( reconnect_info );
			//	}
			//	m_reconnect_info_lock.unlock();
			//	if( false == in_reconnect_list ) {
			//		FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 连接 {0}:{1}:{2} 主动关闭，转入重连队列！", connect_info->m_adress_r, connect_info->m_port_r, connect_info->m_node_type );
			//		LogPrint( syslog_level::c_warn, m_log_cate, log_info );
			//	}
			//}

			// 目前不进行删除
			m_disconnect_info_lock.lock();
			connect_info->clear();
			m_list_disconnect_info.push_back( connect_info );
			m_disconnect_info_lock.unlock();
		}
	}

	void NetClient_P::Client_CloseAll() { // 主动关闭
		m_remote_info_lock.lock();
		std::list<ConnectInfo*> list_remote_info = m_list_remote_info;
		m_remote_info_lock.unlock();

		for( auto it_ci = list_remote_info.begin(); it_ci != list_remote_info.end(); it_ci++ ) {
			Client_Close( *it_ci );
		}
	}

	void NetClient_P::Client_Close( ConnectInfo* connect_info ) {
		if( connect_info && connect_info->m_socket.get() ) {
			connect_info->m_active_close = true; // 标记
			connect_info->m_socket->shutdown( boost::asio::ip::tcp::socket::shutdown_both, boost::system::error_code() );
		}
	}

	void NetClient_P::Client_Close( int32_t identity ) {
		ConnectInfo* connect_info = nullptr;

		m_remote_info_index_lock.lock();
		auto it_ci = m_map_remote_info_index.find( identity );
		if( it_ci != m_map_remote_info_index.end() ) {
			connect_info = it_ci->second;
		}
		m_remote_info_index_lock.unlock();

		if( connect_info ) {
			Client_Close( connect_info );
		}
	}

	void NetClient_P::SendHeartCheck() {
		time_t now_time_t;
		time( &now_time_t );
		m_remote_info_lock.lock();
		std::list<ConnectInfo*> list_remote_info = m_list_remote_info;
		m_remote_info_lock.unlock();

		for( auto it_ci = list_remote_info.begin(); it_ci != list_remote_info.end(); it_ci++ ) {
			if( difftime( now_time_t, (*it_ci)->m_heart_check_time ) >= m_heart_check_time ) { // m_heart_check_time 秒内没发送过数据
				Client_SendData( *it_ci, NW_MSG_TYPE_HEART_CHECK, NW_MSG_CODE_NONE, std::string( "" ) );
			}
		}
		std::string log_info;
		for( auto it_ci = list_remote_info.begin(); it_ci != list_remote_info.end(); it_ci++ ) {
			if( (*it_ci)->m_stat_lost_msg > 0 ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "连接 {0} 丢失 {1} 消息！", (*it_ci)->m_endpoint_r, (*it_ci)->m_stat_lost_msg );
				LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				(*it_ci)->m_stat_lost_msg = 0;
			}
		}
	}

	void NetClient_P::MakeReconnect() {
		if( false == m_auto_reconnect_client ) { // 避免界面无连接存在时一直尝试重连，即可手动取消重连
			m_reconnect_info_lock.lock();
			m_list_reconnect_info.clear();
			m_reconnect_info_lock.unlock();
			return;
		}

		std::list<ReconnectInfo> list_reconnect_info; // 拷贝一份减少锁开销

		m_reconnect_info_lock.lock();
		for( auto it_ri = m_list_reconnect_info.begin(); it_ri != m_list_reconnect_info.end(); it_ri++ ) {
			it_ri->m_reconnect_count++; //
			list_reconnect_info.push_back( *it_ri );
		}
		m_reconnect_info_lock.unlock();

		for( auto it_ri = list_reconnect_info.begin(); it_ri != list_reconnect_info.end(); it_ri++ ) {
			Client_AddConnect( it_ri->m_address, it_ri->m_port, it_ri->m_node_type );
			// 这里看不出是否连接成功的，还要看 Client_HandleConnect() 是否报错，等 Client_AddNewInfo() 再来处理
			std::string log_info;
			FormatLibrary::StandardLibrary::FormatTo( log_info, "客户端 尝试 {0}:{1}:{2} 的第 {3} 次重连。", it_ri->m_address, it_ri->m_port, it_ri->m_node_type, it_ri->m_reconnect_count );
			LogPrint( syslog_level::c_info, m_log_cate, log_info );
		}
	}

	size_t NetClient_P::Client_GetConnectCount() {
		return m_list_remote_info.size();
	}

	ConnectInfo* NetClient_P::Client_GetConnect( int32_t identity ) {
		ConnectInfo* connect_info = nullptr;

		m_remote_info_index_lock.lock();
		auto it_ci = m_map_remote_info_index.find( identity );
		if( it_ci != m_map_remote_info_index.end() ) {
			connect_info = it_ci->second;
		}
		m_remote_info_index_lock.unlock();

		return connect_info;
	}

	NetClient::NetClient()
		: m_net_client_p( nullptr ) {
		try {
			m_net_client_p = new NetClient_P();
		}
		catch( ... ) {}
	}

	NetClient::~NetClient() {
		if( m_net_client_p != nullptr ) {
			delete m_net_client_p;
			m_net_client_p = nullptr;
		}
	}

	void NetClient::ComponentInstance( NetClient_X* net_client_x ) {
		m_net_client_p->ComponentInstance( net_client_x );
	}

	void NetClient::StartNetwork( NetClientCfg& config ) {
		m_net_client_p->StartNetwork( config );
	}

	bool NetClient::IsNetworkStarted() {
		return m_net_client_p->IsNetworkStarted();
	}

	bool NetClient::IsConnectAvailable( ConnectInfo* connect_info ) {
		return m_net_client_p->IsConnectAvailable( connect_info );
	}

	bool NetClient::Client_CanAddConnect() {
		return m_net_client_p->Client_CanAddConnect();
	}

	bool NetClient::Client_CanAddServer( std::string address_r, int32_t port_r ) {
		return m_net_client_p->Client_CanAddServer( address_r, port_r );
	}

	bool NetClient::Client_AddConnect( std::string address_r, int32_t port_r, std::string node_type_r ) {
		return m_net_client_p->Client_AddConnect( address_r, port_r, node_type_r );
	}

	void NetClient::Client_SetAutoReconnect( bool auto_reconnect ) {
		m_net_client_p->Client_SetAutoReconnect( auto_reconnect );
	}

	int32_t NetClient::Client_SendDataAll( int32_t type, int32_t code, std::string& data ) {
		return m_net_client_p->Client_SendDataAll( type, code, data );
	}

	int32_t NetClient::Client_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data ) {
		return m_net_client_p->Client_SendData( connect_info, type, code, data );
	}

	void NetClient::Client_CloseAll() {
		m_net_client_p->Client_CloseAll();
	}

	void NetClient::Client_Close( ConnectInfo* connect_info ) {
		m_net_client_p->Client_Close( connect_info );
	}

	void NetClient::Client_Close( int32_t identity ) {
		m_net_client_p->Client_Close( identity );
	}

	size_t NetClient::Client_GetConnectCount() {
		return m_net_client_p->Client_GetConnectCount();
	}

	ConnectInfo* NetClient::Client_GetConnect( int32_t identity ) {
		return m_net_client_p->Client_GetConnect( identity );
	}

} // namespace basicx
