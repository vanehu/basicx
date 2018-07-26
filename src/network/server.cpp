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

#include <chrono>
#include <iomanip> // std::setw
#include <iostream>

#include "server_.h"

namespace basicx {

	NetServerInfo::NetServerInfo( int32_t info_type, std::string& node_type, int32_t identity, std::string& endpoint_l, std::string& endpoint_r )
		: m_info_type( info_type )
		, m_node_type( node_type )
		, m_identity( identity )
		, m_endpoint_l( endpoint_l )
		, m_endpoint_r( endpoint_r ) {
	}

	NetServerData::NetServerData( std::string& node_type, int32_t identity, int32_t code, std::string& data )
		: m_node_type( node_type )
		, m_identity( identity )
		, m_code( code ) {
		m_data = std::move( data ); // ������ʹ�� std::string& m_data; Ȼ�� m_data( data ) һ��
	}

	NetServer_X::NetServer_X() {
	}

	NetServer_X::~NetServer_X() {
	}

	NetServer_P::NetServer_P()
		: m_net_server_x( nullptr )
		, m_network_running( false )
		, m_identity_count( 0 )
		, m_unique_lock( m_sender_lock )
		, m_sender_vector_1( nullptr )
		, m_sender_vector_2( nullptr )
		, m_sender_vector_read( nullptr )
		, m_sender_vector_write( nullptr )
		, m_sender_running( false )
		, m_log_test( 0 )
		, m_heart_check_time( 10 )
		, m_max_msg_cache_number( 0 ) // 0 Ϊ���޻�������
		, m_io_work_thread_number( 1 )
		, m_client_connect_timeout( 2000 )
		, m_max_connect_total_s( 1000 )
		, m_max_data_length_s( 102400 )
		, m_total_local_connect( 0 )
		, m_log_cate( "<NET_SERVER>" ) {
		m_syslog = SysLog_S::GetInstance();
	}

	NetServer_P::~NetServer_P() {
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

		for( auto it_ci = m_list_local_info.begin(); it_ci != m_list_local_info.end(); it_ci++ ) {
			if( (*it_ci) != nullptr ) {
				(*it_ci)->clear();
				delete (*it_ci);
				(*it_ci) = nullptr;
			}
		}

		for( auto it_li = m_map_listen_info.begin(); it_li != m_map_listen_info.end(); it_li++ ) {
			if( it_li->second != nullptr ) {
				delete it_li->second;
				it_li->second = nullptr;
			}
		}

		for( auto it_di = m_list_disconnect_info.begin(); it_di != m_list_disconnect_info.end(); it_di++ ) {
			if( (*it_di) != nullptr ) {
				// (*it_di)->clear(); // ����ʱ������
				delete (*it_di);
				(*it_di) = nullptr;
			}
		}
	}

	void NetServer_P::ComponentInstance( NetServer_X* net_server_x ) {
		m_net_server_x = net_server_x;
	}

	void NetServer_P::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show/* = 0*/ ) {
		m_syslog->LogWrite( log_level, log_cate, log_info );
		m_syslog->LogPrint( log_level, log_cate, "LOG>: " + log_info ); // ����̨
	}

	void NetServer_P::Thread_ServerOnTime() {
		std::string log_info;

		log_info = "����� ��������ͨ�Ŷ�ʱ�߳����, ��ʼ��������ͨ�Ŷ�ʱ ...";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		try {
			while( true ) {
				std::this_thread::sleep_for( std::chrono::milliseconds( m_heart_check_time * 1000 ) );
				if( IsNetworkStarted() ) {
					SendHeartCheck(); // �������
				}
			}
		} // try
		catch( ... ) {
			log_info = "����� ����ͨ�Ŷ�ʱ�̷߳���δ֪����";
			LogPrint( syslog_level::c_fatal, m_log_cate, log_info );
		}

		log_info = "����� ����ͨ�Ŷ�ʱ�߳��˳���";
		LogPrint( syslog_level::c_warn, m_log_cate, log_info );
	}

	void NetServer_P::Thread_NetServer() {
		std::string log_info;

		log_info = "����� ��������ͨ�ŷ����߳����, ��ʼ��������ͨ�ŷ��� ...";
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

				for( size_t i = 0; i < m_vec_thread.size(); i++ ) { // �ȴ������߳��˳�
					m_vec_thread[i]->join();
				}
			}
			catch( std::exception& ex ) {
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ����ͨ�ŷ��� ��ʼ�� �쳣��{0}", ex.what() );
				}
				else {
					log_info = "����� ����ͨ�ŷ��� ��ʼ�� �쳣��";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		} // try
		catch( ... ) {
			log_info = "����� ����ͨ�ŷ����̷߳���δ֪����";
			LogPrint( syslog_level::c_fatal, m_log_cate, log_info );
		}

		if( true == m_network_running ) {
			m_network_running = false;
			m_service->stop();
		}

		log_info = "����� ����ͨ�ŷ����߳��˳���";
		LogPrint( syslog_level::c_warn, m_log_cate, log_info );
	}

	void NetServer_P::StartNetwork( NetServerCfg& config ) {
		std::string log_info;

		m_log_test = config.m_log_test;
		m_heart_check_time = config.m_heart_check_time;
		m_max_msg_cache_number = config.m_max_msg_cache_number;
		m_io_work_thread_number = config.m_io_work_thread_number;
		m_client_connect_timeout = config.m_client_connect_timeout;
		// ����˲���
		m_max_connect_total_s = config.m_max_connect_total_s;
		m_max_data_length_s = config.m_max_data_length_s;

		m_thread_net_server = std::thread( &NetServer_P::Thread_NetServer, this );
		log_info = "����� ���� ����ͨ�ŷ��� �̡߳�";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		m_thread_server_on_time = std::thread( &NetServer_P::Thread_ServerOnTime, this );
		log_info = "����� ���� ����ͨ�Ŷ�ʱ �̡߳�";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		// ���н���ģʽ�£�Ҫ�������� while() �Ƿ�ᵼ�� LogPrint() �е� SendMessage ���������߱�Ǵ��ڴ������ǰ��Ҫ���ͽ�����Ϣ
		while( false == m_network_running ) {
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}

		m_sender_vector_1 = new SenderVector( m_max_msg_cache_number );
		m_sender_vector_2 = new SenderVector( m_max_msg_cache_number );
		// ����ʵ�ʻ���������ָ����ֵ������
		m_sender_vector_read = m_sender_vector_1;
		m_sender_vector_write = m_sender_vector_1;
		m_sender_running = true;
		m_sender_thread = std::thread( &NetServer_P::Server_HandleSendMsgs, this );
		log_info = "����� ���� ����ͨ�Ŵ��� �̡߳�";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );
	}

	bool NetServer_P::IsNetworkStarted() {
		return m_network_running;
	}

	bool NetServer_P::IsConnectAvailable( ConnectInfo* connect_info ) {
		if( connect_info != nullptr ) {
			return connect_info->m_available;
		}
		return false;
	}

	// int32_t->2147483647��(000000001~214748364)*10+2��NetServer��2
	int32_t NetServer_P::Server_GetIdentity() {
		if( 214748364 == m_identity_count ) {
			m_identity_count = 0;
		}
		m_identity_count++; //
		return m_identity_count * 10 + 2; // NetServer��2
	}

	bool NetServer_P::Server_CanAddConnect() {
		return m_total_local_connect < m_max_connect_total_s;
	}

	bool NetServer_P::Server_CanAddListen( std::string address_l, int32_t port_l ) {
		std::string listen_endpoint;
		FormatLibrary::StandardLibrary::FormatTo( listen_endpoint, "tcp://0.0.0.0:{0}", port_l ); // 0.0.0.0

		bool can_add_listen = true;

		m_listen_info_lock.lock();
		auto it_li = m_map_listen_info.find( listen_endpoint );
		if( it_li != m_map_listen_info.end() ) { // �Ѵ��ڼ���
			can_add_listen = false;
		}
		m_listen_info_lock.unlock();

		return can_add_listen;
	}

	bool NetServer_P::Server_AddListen( std::string address_l, int32_t port_l, std::string node_type_l ) {
		std::string log_info;

		ConnectInfo* connect_info = nullptr;

		try {
			if( !IsNetworkStarted() ) {
				log_info = "����� ����ͨ�ŷ���δ������";
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				return false;
			}

			if( !Server_CanAddListen( address_l, port_l ) ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���ؼ��� {0}:{1} �Ѵ��ڣ�", address_l, port_l );
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				return false;
			}

			// ת��ΪIP��ַ
			boost::asio::ip::tcp::endpoint endpoint( boost::asio::ip::tcp::v4(), port_l );

			connect_info = new ConnectInfo();
			connect_info->m_socket = std::make_shared<boost::asio::ip::tcp::socket>( *m_service );
			connect_info->m_available = false;
			connect_info->m_node_type = node_type_l; //

			AcceptorPtr acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>( *m_service, endpoint );

			// ��¼���ؼ�����Ϣ
			std::string listen_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( listen_endpoint, "tcp://0.0.0.0:{0}", port_l ); // 0.0.0.0
			m_listen_info_lock.lock();
			auto it_li = m_map_listen_info.find( listen_endpoint );
			if( it_li == m_map_listen_info.end() ) {
				ListenInfo* listen_info = new ListenInfo();
				listen_info->m_address = "0.0.0.0"; // 0.0.0.0
				listen_info->m_port = port_l;
				listen_info->m_node_type = node_type_l;
				listen_info->m_connect_number = 0;
				listen_info->m_acceptor = acceptor;
				m_map_listen_info.insert( std::pair<std::string, ListenInfo*>( listen_endpoint, listen_info ) );
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� ���ؼ�����{0}:{1}:{2}", address_l, port_l, node_type_l );
				LogPrint( syslog_level::c_info, m_log_cate, log_info );
			}
			m_listen_info_lock.unlock();

			// �첽��������
			acceptor->async_accept( *(connect_info->m_socket), boost::bind( &NetServer_P::Server_HandleAccept, this, boost::asio::placeholders::error, connect_info, acceptor ) );
		}
		catch( std::exception& ex ) {
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� ���� �쳣��{0}", ex.what() );
			}
			else {
				log_info = "����� ���� ���� �쳣��";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );

			if( connect_info ) {
				if( connect_info->m_socket.get() ) {
					connect_info->m_socket->close();
				}

				delete connect_info;
				connect_info = nullptr;
			}

			return false;
		}

		return true;
	}

	void NetServer_P::Server_HandleAccept( const boost::system::error_code& error, ConnectInfo* connect_info, AcceptorPtr acceptor ) {
		std::string log_info;

		if( !error ) {
			connect_info->m_identity = Server_GetIdentity();
			connect_info->m_available = true;
			time( &connect_info->m_heart_check_time );
			connect_info->m_active_close = false;

			connect_info->m_protocol_r = "tcp";
			connect_info->m_adress_r = connect_info->m_socket->remote_endpoint().address().to_string().c_str();
			connect_info->m_port_r = connect_info->m_socket->remote_endpoint().port();
			FormatLibrary::StandardLibrary::FormatTo( connect_info->m_str_port_r, "{0}", connect_info->m_port_r );
			connect_info->m_endpoint_r = connect_info->m_protocol_r + "://" + connect_info->m_adress_r + ":" + connect_info->m_str_port_r; // ���ӱ����"tcp://192.16.1.23:333"

			connect_info->m_protocol_l = "tcp";
			connect_info->m_adress_l = connect_info->m_socket->local_endpoint().address().to_string().c_str();
			connect_info->m_port_l = connect_info->m_socket->local_endpoint().port();
			FormatLibrary::StandardLibrary::FormatTo( connect_info->m_str_port_l, "{0}", connect_info->m_port_l );
			connect_info->m_endpoint_l = connect_info->m_protocol_l + "://" + connect_info->m_adress_l + ":" + connect_info->m_str_port_l; // ���ӱ����"tcp://192.16.1.38:465"

			connect_info->m_recv_buf_head = new char[HEAD_BYTES];
			connect_info->m_recv_buf_data = new char[1024];
			connect_info->m_recv_buf_data_size = 1024;

			connect_info->m_stat_lost_msg = 0;

			if( !Server_CanAddConnect() ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ȫ������ {0}:{1} �������ܾ����ӣ�[{2}]<-[{3}]", m_total_local_connect, m_max_connect_total_s, connect_info->m_endpoint_l, connect_info->m_endpoint_r );
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				std::string node_type = connect_info->m_node_type;
				if( connect_info ) {
					if( connect_info->m_socket.get() ) {
						connect_info->m_socket->close();
					}

					delete connect_info;
					connect_info = nullptr;
				}
				// ׼�������µ���������
				Server_KeepOnAccept( node_type, acceptor );
				return;
			}

			m_local_info_lock.lock();
			m_list_local_info.push_back( connect_info );
			m_total_local_connect = m_list_local_info.size(); //
			m_local_info_lock.unlock();
			m_local_info_index_lock.lock();
			m_map_local_info_index.insert( std::pair<int32_t, ConnectInfo*>( connect_info->m_identity, connect_info ) );
			m_local_info_index_lock.unlock();

			FormatLibrary::StandardLibrary::FormatTo( log_info, "����� �������ӣ�[{0}]<-[{1}]��{2}", connect_info->m_endpoint_l, connect_info->m_endpoint_r, connect_info->m_identity );
			LogPrint( syslog_level::c_info, m_log_cate, log_info );

			// ���±��ؼ�����Ϣ
			std::string listen_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( listen_endpoint, "tcp://0.0.0.0:{0}", connect_info->m_port_l ); // 0.0.0.0
			m_listen_info_lock.lock();
			auto it_li = m_map_listen_info.find( listen_endpoint );
			if( it_li != m_map_listen_info.end() ) {
				it_li->second->m_connect_number++;
			}
			m_listen_info_lock.unlock();

			try {
				connect_info->m_socket->set_option( boost::asio::ip::tcp::no_delay( m_tcp_socket_option.m_no_delay ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::keep_alive( m_tcp_socket_option.m_keep_alive ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::enable_connection_aborted( m_tcp_socket_option.m_enable_connection_aborted ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::receive_buffer_size( m_tcp_socket_option.m_recv_buffer_size ) );
				connect_info->m_socket->set_option( boost::asio::socket_base::send_buffer_size( m_tcp_socket_option.m_send_buffer_size ) );

				// ׼���ӿͻ��˽��� Type ����
				memset( connect_info->m_recv_buf_head, 0, HEAD_BYTES );
				boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_head, HEAD_BYTES ), boost::bind( &NetServer_P::Server_HandleRecvHead, this, boost::asio::placeholders::error, connect_info ) );

				// ׼�������µ���������
				Server_KeepOnAccept( connect_info->m_node_type, acceptor );
			}
			catch( std::exception& ex ) {
				Server_CloseOnError( connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� ���� �쳣��{0}", ex.what() );
				}
				else {
					log_info = "����� ���� ���� �쳣��";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}

			if( nullptr != m_net_server_x ) {
				NetServerInfo net_server_info( WM_MY_NEWCONNECT_LOCAL, connect_info->m_node_type, connect_info->m_identity, connect_info->m_endpoint_l, connect_info->m_endpoint_r ); // ����ͨ��ģ���յ��µĴӿͻ�������
				m_net_server_x->OnNetServerInfo( net_server_info );
			}
		}
		else {
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� ���� ����{0}", error.message().c_str() );
			}
			else {
				log_info = "����� ���� ���� ����";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );

			if( connect_info ) {
				if( connect_info->m_socket.get() ) {
					connect_info->m_socket->close();
				}

				delete connect_info;
				connect_info = nullptr;
			}
		}
	}

	void NetServer_P::Server_KeepOnAccept( std::string node_type_l, AcceptorPtr acceptor ) {
		ConnectInfo* connect_info = nullptr;

		try {
			connect_info = new ConnectInfo();
			connect_info->m_socket = std::make_shared<boost::asio::ip::tcp::socket>( *m_service );
			connect_info->m_available = false;
			connect_info->m_node_type = node_type_l; //

			// �첽��������
			acceptor->async_accept( *(connect_info->m_socket), boost::bind( &NetServer_P::Server_HandleAccept, this, boost::asio::placeholders::error, connect_info, acceptor ) );
		}
		catch( std::exception& ex ) {
			std::string log_info;
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� ���� �쳣��{0}", ex.what() );
			}
			else {
				log_info = "����� ���� ���� �쳣��";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );

			if( connect_info ) {
				if( connect_info->m_socket.get() ) {
					connect_info->m_socket->close();
				}

				delete connect_info;
				connect_info = nullptr;
			}
		}
	}

	void NetServer_P::Server_HandleRecvHead( const boost::system::error_code& error, ConnectInfo* connect_info ) {
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
					FormatLibrary::StandardLibrary::FormatTo( log_info, "<--[{0}]��Type:{1} Code:{2} Size:{3}", connect_info->m_endpoint_r, type, code, size );
					LogPrint( syslog_level::c_info, m_log_cate, log_info );
				}

				if( type < NW_MSG_ATOM_TYPE_MIN || type > NW_MSG_USER_TYPE_MAX ) {
					std::string log_info;
					Server_CloseOnError( connect_info );
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� �յ� δ֪�����ݰ����ͣ�{0}", type );
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}

				if( code < NW_MSG_CODE_TYPE_MIN || code > NW_MSG_CODE_TYPE_MAX ) {
					std::string log_info;
					Server_CloseOnError( connect_info );
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� �յ� δ֪�����ݰ����룺{0}", code );
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}

				if( 0 == size ) {
					// ׼���ӿͻ��˽��� Type ����
					memset( connect_info->m_recv_buf_head, 0, HEAD_BYTES );
					boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_head, HEAD_BYTES ), boost::bind( &NetServer_P::Server_HandleRecvHead, this, boost::asio::placeholders::error, connect_info ) );
				}
				else if( size > 0 && size <= m_max_data_length_s ) {
					// ׼���ӿͻ��˽��� Data ����
					if( connect_info->m_recv_buf_data_size < size ) {
						delete[] connect_info->m_recv_buf_data;
						connect_info->m_recv_buf_data = new char[size];
						connect_info->m_recv_buf_data_size = size;
					}
					memset( connect_info->m_recv_buf_data, 0, size );
					boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_data, size ), boost::bind( &NetServer_P::Server_HandleRecvData, this, boost::asio::placeholders::error, connect_info, type, code, size ) );
				}
				else {
					std::string log_info;
					Server_CloseOnError( connect_info );
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� �յ� �쳣�����ݰ���С��{0}", size );
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Server_CloseOnError( connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Head ���� �쳣��{0}", ex.what() );
				}
				else {
					log_info = "����� ���� Head ���� �쳣��";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		}
		else {
			std::string log_info;
			Server_CloseOnError( connect_info );
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Head ���� ����{0}", error.message().c_str() );
			}
			else {
				log_info = "����� ���� Head ���� ����";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	void NetServer_P::Server_HandleRecvData( const boost::system::error_code& error, ConnectInfo* connect_info, int32_t type, int32_t code, int32_t size ) {
		if( !error ) {
			try {
				std::string data( connect_info->m_recv_buf_data, size );

				if( 1 == m_log_test ) {
					std::string log_info;
					FormatLibrary::StandardLibrary::FormatTo( log_info, "<--[{0}]��Data:{1}", connect_info->m_endpoint_r, data.length() );
					LogPrint( syslog_level::c_info, m_log_cate, log_info );
				}

				if( !data.empty() ) {
					try {
						if( m_net_server_x != nullptr ) {
							NetServerData net_server_data( connect_info->m_node_type, connect_info->m_identity, code, data ); // std::move( data )
							m_net_server_x->OnNetServerData( net_server_data );
						}
						else {
							std::string log_info;
							log_info = "����ͨ�ŵ���ģ��ָ�� m_net_server_x Ϊ�գ�";
							LogPrint( syslog_level::c_warn, m_log_cate, log_info );
						}
					}
					catch( ... ) {
						std::string log_info;
						log_info = "����� ���� Data ���� ת������ ����δ֪����";
						LogPrint( syslog_level::c_error, m_log_cate, log_info );
					}

					// ׼���ӿͻ��˽��� Type ����
					memset( connect_info->m_recv_buf_head, 0, HEAD_BYTES );
					boost::asio::async_read( *(connect_info->m_socket), boost::asio::buffer( connect_info->m_recv_buf_head, HEAD_BYTES ), boost::bind( &NetServer_P::Server_HandleRecvHead, this, boost::asio::placeholders::error, connect_info ) );
				}
				else {
					std::string log_info;
					Server_CloseOnError( connect_info );
					log_info = "����� �յ� ���ݰ�����Ϊ�գ�";
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Server_CloseOnError( connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Data ���� �쳣��{0}", ex.what() );
				}
				else {
					log_info = "����� ���� Data ���� �쳣��";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		}
		else {
			std::string log_info;
			Server_CloseOnError( connect_info );
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Data ���� ����{0}", error.message().c_str() );
			}
			else {
				log_info = "����� ���� Data ���� ����";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	int32_t NetServer_P::Server_SendDataAll( int32_t type, int32_t code, std::string& data ) {
		m_local_info_lock.lock();
		std::list<ConnectInfo*> list_local_info = m_list_local_info;
		m_local_info_lock.unlock();
		int32_t send_count = 0;
		for( auto it_ci = list_local_info.begin(); it_ci != list_local_info.end(); it_ci++ ) {
			if( 0 == Server_SendData( *it_ci, type, code, data ) ) {
				send_count++;
			}
		}
		return send_count;
	}

	int32_t NetServer_P::Server_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data ) {
		int32_t result = 0;
		if( connect_info != nullptr && connect_info->m_available != false ) {
			size_t size = data.length();
			if( size <= m_max_data_length_s ) {
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
				//FormatLibrary::StandardLibrary::FormatTo( log_info, "AddSend��{0} {1} {2} {3}", str_type.c_str(), str_code.c_str(), str_size.c_str(), data.c_str() );
				//LogPrint( syslog_level::c_info, m_log_cate, log_info );

				m_writing_vector_lock.lock();
				try {
					bool sender_is_full = false;
					if( m_sender_vector_write->m_count == m_sender_vector_write->m_capacity ) { // �軻���У�����д�� capacity �ſ��Ǹ���
						m_changing_vector_lock.lock();
						if( m_sender_vector_write == m_sender_vector_read ) { // ���߳�δռ��Ŀ����У������л�
							m_sender_vector_write = m_sender_vector_write == m_sender_vector_1 ? m_sender_vector_2 : m_sender_vector_1; // �л�����
							m_sender_vector_write->m_count = 0; // ���¿�ʼ���ڶ��м���
						}
						else {
							if( 0 == m_max_msg_cache_number ) { // ���޻�������
								m_sender_vector_write->m_capacity += m_sender_vector_write->m_capacity; // ���ӵ������ȣ��ȴ����߳��뿪Ŀ�����
							}
							else { // ���ƻ�������
								sender_is_full = true;
								connect_info->m_stat_lost_msg++;
								result = -1;
							}
						}
						m_changing_vector_lock.unlock();
					}
					if( false == sender_is_full ) {
						if( m_sender_vector_write->m_count < m_sender_vector_write->m_vec_send_buf_info.size() ) { // ʹ�þ���Ԫ��
							m_sender_vector_write->m_vec_send_buf_info[m_sender_vector_write->m_count]->Update( connect_info, size, type_temp, code_temp, size_temp, data );
							m_sender_vector_write->m_count++; // �����Ժ�
						}
						else { // ʹ���½�Ԫ��
							m_sender_vector_write->m_vec_send_buf_info.push_back( new SendBufInfo( connect_info, size, type_temp, code_temp, size_temp, data ) );
							m_sender_vector_write->m_count++; // ����Ժ�
						}
						m_sender_condition.notify_all(); //
					}
				}
				catch( std::exception& ex ) {
					std::string log_info;
					if( 1 == m_log_test ) {
						FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ��� Data ��Ϣ �쳣��{0}", ex.what() );
					}
					else {
						log_info = "����� ��� Data ��Ϣ �쳣��";
					}
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
					result = -2;
				}
				m_writing_vector_lock.unlock();
			}
			else {
				std::string log_info;
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ��� Data ��Ϣ ���������{0} > {1}", size, m_max_data_length_s );
				LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				result = -3;
			}
		}
		else {
			//std::string log_info = "����� ��� Data ��Ϣ �����ѶϿ���";
			//LogPrint( syslog_level::c_warn, m_log_cate, log_info );
			result = -4;
		}
		return result;
	}

	void NetServer_P::Server_HandleSendMsgs() {
		while( true == m_sender_running ) {
			m_sender_condition.wait( m_unique_lock );
			for( ; m_sender_vector_read->m_handled < m_sender_vector_read->m_count; ) {
				m_sender_vector_read->m_handled++;
				SendBufInfo* send_buf_info = m_sender_vector_read->m_vec_send_buf_info[m_sender_vector_read->m_handled - 1];
				try {
					if( send_buf_info->m_connect_info != nullptr && send_buf_info->m_connect_info->m_available != false ) { // ���룡��Ȼ���������ѶϿ�����ȥ�������ݣ���ȡ�˻ᷢ�������ѶϿ���������������
						boost::asio::async_write( *( send_buf_info->m_connect_info->m_socket ), boost::asio::buffer( send_buf_info->m_send_buf ), boost::bind( &NetServer_P::Server_HandleSendData, this, boost::asio::placeholders::error, send_buf_info ) );
					}
				}
				catch( std::exception& ex ) {
					std::string log_info;
					Server_CloseOnError( send_buf_info->m_connect_info );
					if( 1 == m_log_test ) {
						FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Msgs ���� �쳣��{0}", ex.what() );
					}
					else {
						log_info = "����� ���� Msgs ���� �쳣��";
					}
					LogPrint( syslog_level::c_error, m_log_cate, log_info );
				}
			}
			while( m_sender_vector_read->m_handled == m_sender_vector_read->m_capacity ) { // �軻���У�������� capacity �ſ��Ǹ���
				bool changed = false;
				m_changing_vector_lock.lock();
				if( m_sender_vector_read != m_sender_vector_write ) { // д�߳�����Ŀ����У������л�
					m_sender_vector_read = m_sender_vector_read == m_sender_vector_1 ? m_sender_vector_2 : m_sender_vector_1; // �л�����
					m_sender_vector_read->m_handled = 0; // ���¿�ʼ���ڶ��м���
					changed = true; //
				}
				// else {} // �ȴ�д�߳�����Ŀ�����
				m_changing_vector_lock.unlock();
				if( true == changed ) { // �ѻ����¶���
					for( ; m_sender_vector_read->m_handled < m_sender_vector_read->m_count; ) {
						m_sender_vector_read->m_handled++;
						SendBufInfo* send_buf_info = m_sender_vector_read->m_vec_send_buf_info[m_sender_vector_read->m_handled - 1];
						try {
							if( send_buf_info->m_connect_info != nullptr && send_buf_info->m_connect_info->m_available != false ) { // ���룡��Ȼ���������ѶϿ�����ȥ�������ݣ���ȡ�˻ᷢ�������ѶϿ���������������
								boost::asio::async_write( *( send_buf_info->m_connect_info->m_socket ), boost::asio::buffer( send_buf_info->m_send_buf ), boost::bind( &NetServer_P::Server_HandleSendData, this, boost::asio::placeholders::error, send_buf_info ) );
							}
						}
						catch( std::exception& ex ) {
							std::string log_info;
							Server_CloseOnError( send_buf_info->m_connect_info );
							if( 1 == m_log_test ) {
								FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Msgs ���� �쳣��{0}", ex.what() );
							}
							else {
								log_info = "����� ���� Msgs ���� �쳣��";
							}
							LogPrint( syslog_level::c_error, m_log_cate, log_info );
						}
					}
				}
				// �ڴ�����Ϣ�����У�д�߳̿����ְѶ���д���ˣ����Բ����Ƿ�����л�
			}
		}
	}

	void NetServer_P::Server_HandleSendData( const boost::system::error_code& error, SendBufInfo* send_buf_info ) {
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
					FormatLibrary::StandardLibrary::FormatTo( log_info, "-->[{0}]��Type:{1} Code:{2} Size:{3} Data:{4}", send_buf_info->m_connect_info->m_endpoint_r, type, code, size, data.length() );
					LogPrint( syslog_level::c_info, m_log_cate, log_info );
				}
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Server_CloseOnError( send_buf_info->m_connect_info );
				if( 1 == m_log_test ) {
					FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Data ���� �쳣��{0}", ex.what() );
				}
				else {
					log_info = "����� ���� Data ���� �쳣��";
				}
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		}
		else {
			std::string log_info;
			Server_CloseOnError( send_buf_info->m_connect_info );
			if( 1 == m_log_test ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Data ���� ����{0}", error.message().c_str() );
			}
			else {
				log_info = "����� ���� Data ���� ����";
			}
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	//int32_t NetServer_P::Server_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data ) {
	//	int32_t result = 0;

	//	if( connect_info != nullptr && connect_info->m_available != false ) {
	//		try {
	//			size_t size = data.length();
	//			if( size <= m_max_data_length_s ) {
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
	//				//FormatLibrary::StandardLibrary::FormatTo( log_info, "AddSend��{0} {1} {2} {3}", str_type.c_str(), str_code.c_str(), str_size.c_str(), data.c_str() );
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
	//					m_service->post( boost::bind( &NetServer_P::Server_HandleSendMsgs, this, connect_info ) );
	//				}

	//				result = 1;
	//			}
	//			else {
	//				std::string log_info;
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ��� Data ��Ϣ ���������{0} > {1}", size, m_max_data_length_s );
	//				LogPrint( syslog_level::c_warn, m_log_cate, log_info );
	//				result = -2;
	//			}
	//		}
	//		catch( std::exception& ex ) {
	//			result = -1;
	//			std::string log_info;
	//			if( 1 == m_log_test ) {
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ��� Data ��Ϣ �쳣��{0}", ex.what() );
	//			}
	//			else {
	//				log_info = "����� ��� Data ��Ϣ �쳣��";
	//			}
	//			LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//		}
	//	}

	//	return result;
	//}

	//void NetServer_P::Server_HandleSendMsgs( ConnectInfo* connect_info ) {
	//	try {
	//		SendBufInfo* send_buf_info = &connect_info->m_list_send_buf_infos.front(); // �϶���
	//		if( true == connect_info->m_available ) { // ���룡��Ȼ���������ѶϿ�����ȥ�������ݣ���ȡ�˻ᷢ�������ѶϿ���������������
	//			boost::asio::async_write( *(connect_info->m_socket), boost::asio::buffer( send_buf_info->m_send_buf ), boost::bind( &NetServer_P::Server_HandleSendData, this, boost::asio::placeholders::error, connect_info ) );
	//		}
	//	}
	//	catch( std::exception& ex ) {
	//		std::string log_info;
	//		Server_CloseOnError( connect_info );
	//		if( 1 == m_log_test ) {
	//			FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Msgs ���� �쳣��{0}", ex.what() );
	//		}
	//		else {
	//			log_info = "����� ���� Msgs ���� �쳣��";
	//		}
	//		LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//	}
	//}

	//void NetServer_P::Server_HandleSendData( const boost::system::error_code& error, ConnectInfo* connect_info ) {
	//	if( !error ) {
	//		try {
	//			SendBufInfo* send_buf_info = &connect_info->m_list_send_buf_infos.front(); // �϶���
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
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "-->[{0}]��Type:{1} Code:{2} Size:{3} Data:{4}", connect_info->m_endpoint_r, type, code, size, data.length() );
	//				LogPrint( syslog_level::c_info, m_log_cate, log_info );
	//			}

	//			connect_info->m_send_buf_info_lock.lock();
	//			connect_info->m_list_send_buf_infos.pop_front();
	//			bool write_in_progress = !connect_info->m_list_send_buf_infos.empty();
	//			connect_info->m_send_buf_info_lock.unlock();

	//			if( write_in_progress && true == m_network_running ) {
	//				m_service->post( boost::bind( &NetServer_P::Server_HandleSendMsgs, this, connect_info ) );
	//			}
	//		}
	//		catch( std::exception& ex ) {
	//			std::string log_info;
	//			Server_CloseOnError( connect_info );
	//			if( 1 == m_log_test ) {
	//				FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Data ���� �쳣��{0}", ex.what() );
	//			}
	//			else {
	//				log_info = "����� ���� Data ���� �쳣��";
	//			}
	//			LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//		}
	//	}
	//	else {
	//		std::string log_info;
	//		Server_CloseOnError( connect_info );
	//		if( 1 == m_log_test ) {
	//			FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� Data ���� ����{0}", error.message().c_str() );
	//		}
	//		else {
	//			log_info = "����� ���� Data ���� ����";
	//		}
	//		LogPrint( syslog_level::c_error, m_log_cate, log_info );
	//	}
	//}

	void NetServer_P::Server_CloseOnError( ConnectInfo* connect_info ) {
		if( connect_info ) {
			if( true == connect_info->m_active_close ) { // �����ر�
				Server_ActiveClose( connect_info );
			}
			else { // �����ر�
				Server_PassiveClose( connect_info );
			}
		}
	}

	void NetServer_P::Server_PassiveClose( ConnectInfo* connect_info ) {
		std::string log_info;

		if( connect_info ) {
			connect_info->m_available = false;

			m_local_info_lock.lock();
			for( auto it_ci = m_list_local_info.begin(); it_ci != m_list_local_info.end(); it_ci++ ) {
				if( (*it_ci)->m_identity == connect_info->m_identity ) {
					m_list_local_info.erase( it_ci );
					m_total_local_connect = m_list_local_info.size(); //
					break;
				}
			}
			m_local_info_lock.unlock();
			m_local_info_index_lock.lock();
			m_map_local_info_index.erase( connect_info->m_identity );
			m_local_info_index_lock.unlock();

			// ���±��ؼ�����Ϣ
			std::string listen_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( listen_endpoint, "tcp://0.0.0.0:{0}", connect_info->m_port_l ); // 0.0.0.0
			m_listen_info_lock.lock();
			auto it_li = m_map_listen_info.find( listen_endpoint );
			if( it_li != m_map_listen_info.end() ) {
				it_li->second->m_connect_number--;
			}
			m_listen_info_lock.unlock();

			if( connect_info->m_socket.get() ) {
				connect_info->m_socket->close();
			}

			FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� �ر����ӡ�[{0}]<-[{1}]", connect_info->m_endpoint_l, connect_info->m_endpoint_r );
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );

			if( nullptr != m_net_server_x ) {
				NetServerInfo net_server_info( WM_MY_DISCONNECT_LOCAL, connect_info->m_node_type, connect_info->m_identity, connect_info->m_endpoint_l, connect_info->m_endpoint_r ); // ����ͨ��ģ���յ��ͻ��˶Ͽ�����
				m_net_server_x->OnNetServerInfo( net_server_info );
			}

			// ����˲������Զ�����

			// Ŀǰ������ɾ��
			m_disconnect_info_lock.lock();
			connect_info->clear();
			m_list_disconnect_info.push_back( connect_info );
			m_disconnect_info_lock.unlock();
		}
	}

	void NetServer_P::Server_ActiveClose( ConnectInfo* connect_info ) {
		std::string log_info;

		if( connect_info ) {
			connect_info->m_available = false;

			m_local_info_lock.lock();
			for( auto it_ci = m_list_local_info.begin(); it_ci != m_list_local_info.end(); it_ci++ ) {
				if( (*it_ci)->m_identity == connect_info->m_identity ) {
					m_list_local_info.erase( it_ci );
					m_total_local_connect = m_list_local_info.size(); //
					break;
				}
			}
			m_local_info_lock.unlock();
			m_local_info_index_lock.lock();
			m_map_local_info_index.erase( connect_info->m_identity );
			m_local_info_index_lock.unlock();

			// ���±��ؼ�����Ϣ
			std::string listen_endpoint;
			FormatLibrary::StandardLibrary::FormatTo( listen_endpoint, "tcp://0.0.0.0:{0}", connect_info->m_port_l ); // 0.0.0.0
			m_listen_info_lock.lock();
			auto it_li = m_map_listen_info.find( listen_endpoint );
			if( it_li != m_map_listen_info.end() ) {
				it_li->second->m_connect_number--;
			}
			m_listen_info_lock.unlock();

			if( connect_info->m_socket.get() ) {
				connect_info->m_socket->close();
			}

			FormatLibrary::StandardLibrary::FormatTo( log_info, "����� ���� �ر����ӡ�[{0}]<-[{1}]", connect_info->m_endpoint_l, connect_info->m_endpoint_r );
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );

			if( nullptr != m_net_server_x ) {
				NetServerInfo net_server_info( WM_MY_CLOSECONNECT_SERVER, connect_info->m_node_type, connect_info->m_identity, connect_info->m_endpoint_l, connect_info->m_endpoint_r ); // ����ͨ��ģ�����������ر�����
				m_net_server_x->OnNetServerInfo( net_server_info );
			}

			// ����˲������Զ�����

			// Ŀǰ������ɾ��
			m_disconnect_info_lock.lock();
			connect_info->clear();
			m_list_disconnect_info.push_back( connect_info );
			m_disconnect_info_lock.unlock();
		}
	}

	void NetServer_P::Server_CloseAll() { // �����ر�
		m_local_info_lock.lock();
		std::list<ConnectInfo*> list_local_info = m_list_local_info;
		m_local_info_lock.unlock();

		for( auto it_ci = list_local_info.begin(); it_ci != list_local_info.end(); it_ci++ ) {
			Server_Close( *it_ci );
		}
	}

	void NetServer_P::Server_Close( ConnectInfo* connect_info ) {
		if( connect_info && connect_info->m_socket.get() ) {
			connect_info->m_active_close = true; // ���
			connect_info->m_socket->shutdown( boost::asio::ip::tcp::socket::shutdown_both, boost::system::error_code() );
		}
	}

	void NetServer_P::Server_Close( int32_t identity ) {
		ConnectInfo* connect_info = nullptr;

		m_local_info_index_lock.lock();
		auto it_ci = m_map_local_info_index.find( identity );
		if( it_ci != m_map_local_info_index.end() ) {
			connect_info = it_ci->second;
		}
		m_local_info_index_lock.unlock();

		if( connect_info ) {
			Server_Close( connect_info );
		}
	}

	void NetServer_P::SendHeartCheck() {
		time_t now_time_t;
		time( &now_time_t );
		m_local_info_lock.lock();
		std::list<ConnectInfo*> list_local_info = m_list_local_info;
		m_local_info_lock.unlock();

		for( auto it_ci = list_local_info.begin(); it_ci != list_local_info.end(); it_ci++ ) {
			if( difftime( now_time_t, (*it_ci)->m_heart_check_time ) >= m_heart_check_time ) { // m_heart_check_time ����û���͹�����
				Server_SendData( *it_ci, NW_MSG_TYPE_HEART_CHECK, NW_MSG_CODE_NONE, std::string( "" ) );
			}
		}
		std::string log_info;
		for( auto it_ci = list_local_info.begin(); it_ci != list_local_info.end(); it_ci++ ) {
			if( (*it_ci)->m_stat_lost_msg > 0 ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "���� {0} ��ʧ {1} ��Ϣ��", (*it_ci)->m_endpoint_r, (*it_ci)->m_stat_lost_msg );
				LogPrint( syslog_level::c_warn, m_log_cate, log_info );
				(*it_ci)->m_stat_lost_msg = 0;
			}
		}
	}

	size_t NetServer_P::Server_GetConnectCount() {
		return m_list_local_info.size();
	}

	ConnectInfo* NetServer_P::Server_GetConnect( int32_t identity ) {
		ConnectInfo* connect_info = nullptr;

		m_local_info_index_lock.lock();
		auto it_ci = m_map_local_info_index.find( identity );
		if( it_ci != m_map_local_info_index.end() ) {
			connect_info = it_ci->second;
		}
		m_local_info_index_lock.unlock();

		return connect_info;
	}

	NetServer::NetServer()
		: m_net_server_p( nullptr ) {
		try {
			m_net_server_p = new NetServer_P();
		}
		catch( ... ) {}
	}

	NetServer::~NetServer() {
		if( m_net_server_p != nullptr ) {
			delete m_net_server_p;
			m_net_server_p = nullptr;
		}
	}

	void NetServer::ComponentInstance( NetServer_X* net_server_x ) {
		m_net_server_p->ComponentInstance( net_server_x );
	}

	void NetServer::StartNetwork( NetServerCfg& config ) {
		m_net_server_p->StartNetwork( config );
	}

	bool NetServer::IsNetworkStarted() {
		return m_net_server_p->IsNetworkStarted();
	}

	bool NetServer::IsConnectAvailable( ConnectInfo* connect_info ) {
		return m_net_server_p->IsConnectAvailable( connect_info );
	}

	bool NetServer::Server_CanAddConnect() {
		return m_net_server_p->Server_CanAddConnect();
	}

	bool NetServer::Server_CanAddListen( std::string address_l, int32_t port_l ) {
		return m_net_server_p->Server_CanAddListen( address_l, port_l );
	}

	bool NetServer::Server_AddListen( std::string address_l, int32_t port_l, std::string node_type_l ) {
		return m_net_server_p->Server_AddListen( address_l, port_l, node_type_l );
	}

	int32_t NetServer::Server_SendDataAll( int32_t type, int32_t code, std::string& data ) {
		return m_net_server_p->Server_SendDataAll( type, code, data );
	}

	int32_t NetServer::Server_SendData( ConnectInfo* connect_info, int32_t type, int32_t code, std::string& data ) {
		return m_net_server_p->Server_SendData( connect_info, type, code, data );
	}

	void NetServer::Server_CloseAll() {
		m_net_server_p->Server_CloseAll();
	}

	void NetServer::Server_Close( ConnectInfo* connect_info ) {
		m_net_server_p->Server_Close( connect_info );
	}

	void NetServer::Server_Close( int32_t identity ) {
		m_net_server_p->Server_Close( identity );
	}

	size_t NetServer::Server_GetConnectCount() {
		return m_net_server_p->Server_GetConnectCount();
	}

	ConnectInfo* NetServer::Server_GetConnect( int32_t identity ) {
		return m_net_server_p->Server_GetConnect( identity );
	}

} // namespace basicx
