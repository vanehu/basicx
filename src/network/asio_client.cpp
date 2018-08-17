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

#include "asio_client_.h"

#include <common/Format/Format.hpp>
#include <syslog/syslog.h>

namespace basicx {

	AsioClient_P::AsioClient_P()
		: m_network_running( false )
		, m_log_cate( "<ASIO_CLIENT>" ) {
		m_syslog = SysLog_S::GetInstance();
	}

	AsioClient_P::~AsioClient_P() {
		if( true == m_network_running ) {
			m_network_running = false;
			m_service->stop();
		}

		for( auto it_di = m_list_disconnect_info.begin(); it_di != m_list_disconnect_info.end(); it_di++ ) {
			if( (*it_di) != nullptr ) {
				delete (*it_di);
				(*it_di) = nullptr;
			}
		}
	}

	void AsioClient_P::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show/* = 0*/ ) {
		m_syslog->LogWrite( log_level, log_cate, log_info );
		m_syslog->LogPrint( log_level, log_cate, "LOG>: " + log_info ); // ����̨
	}

	void AsioClient_P::Thread_AsioClient() {
		std::string log_info;

		log_info = "�ͻ��� ��������ͨ�ŷ����߳����, ��ʼ��������ͨ�ŷ��� ...";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		try {
			try {
				m_service = std::make_shared<boost::asio::io_service>();
				boost::asio::io_service::work work( *m_service );

				ThreadPtr thread_ptr( new std::thread( boost::bind( &boost::asio::io_service::run, m_service ) ) );
				m_network_running = true;
				thread_ptr->join();
			}
			catch( std::exception& ex ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� ����ͨ�ŷ��� ��ʼ�� �쳣��{0}", ex.what() );
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		} // try
		catch( ... ) {
			log_info = "�ͻ��� ����ͨ�ŷ����̷߳���δ֪����";
			LogPrint( syslog_level::c_fatal, m_log_cate, log_info );
		}

		if( true == m_network_running ) {
			m_network_running = false;
			m_service->stop();
		}

		log_info = "�ͻ��� ����ͨ�ŷ����߳��˳���";
		LogPrint( syslog_level::c_warn, m_log_cate, log_info );
	}

	void AsioClient_P::StartNetwork() {
		std::string log_info;

		m_thread_asio_client = std::thread( &AsioClient_P::Thread_AsioClient, this );
		log_info = "�ͻ��� ���� ����ͨ�ŷ��� �̡߳�";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		// ���н���ģʽ�£�Ҫ�������� while() �Ƿ�ᵼ�� LogPrint() �е� SendMessage ���������߱�Ǵ��ڴ������ǰ��Ҫ���ͽ�����Ϣ
		while( false == m_network_running ) {
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}
	}

	bool AsioClient_P::IsNetworkStarted() {
		return m_network_running;
	}

	ConnectAsio* AsioClient_P::Client_AddConnect( std::string address, int32_t port, std::string node_type, int32_t time_out_sec ) {
		std::string log_info;

		ConnectAsio* connect_info = nullptr;

		try {
			if( !IsNetworkStarted() ) {
				log_info = "�ͻ��� ����ͨ�ŷ���δ������";
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				return connect_info;
			}

			std::string port_temp;
			FormatLibrary::StandardLibrary::FormatTo( port_temp, "{0}", port );
			std::string address_temp = address;

			// ת��ΪIP��ַ
			boost::asio::ip::tcp::resolver resolver( *m_service );
			boost::asio::ip::tcp::resolver::query query( address_temp, port_temp );
			boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve( query );

			connect_info = new ConnectAsio; // ������ӽ���������������Ϣ�����ܻ��Ҳ��������ӣ���Ϊ���ﻹû�������
			connect_info->m_socket = std::make_shared<boost::asio::ip::tcp::socket>( *m_service );
			connect_info->m_available = false;
			connect_info->m_node_type = node_type;

			boost::asio::async_connect( *(connect_info->m_socket), endpoint_iterator, boost::bind( &AsioClient_P::Client_HandleConnect, this, boost::asio::placeholders::error, connect_info ) );

			std::unique_lock<std::mutex> wait_locker( connect_info->m_mutex );
			std::cv_status my_cv_status = connect_info->m_condition.wait_for( wait_locker, std::chrono::seconds( time_out_sec ) );
			if( std::cv_status::timeout == my_cv_status ) {
				if( connect_info->m_socket.get() ) {
					connect_info->m_socket->close();
				}

				delete connect_info;
				connect_info = nullptr;

				log_info = "�ͻ��� ���� ���� ��ʱ��";
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
		}
		catch( std::exception& ex ) { // ���û�д�����ʱ�����ѽ�����connect_info �ɳ�ʱ���� Client_CheckConnectTime() �������������ڴ�����
			FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� ���� ���� {0}:{1} �쳣��{2}", address, port, ex.what() );
			LogPrint( syslog_level::c_error, m_log_cate, log_info );

			if( connect_info ) {
				if( connect_info->m_socket.get() ) {
					connect_info->m_socket->close();
				}

				delete connect_info;
				connect_info = nullptr;
			}
		}

		return connect_info;
	}

	void AsioClient_P::Client_HandleConnect( const boost::system::error_code& error, ConnectAsio* connect_info ) {
		std::string log_info;

		if( !error ) {
			connect_info->m_socket->set_option( boost::asio::ip::tcp::no_delay( m_tcp_socket_option.m_no_delay ) );
			connect_info->m_socket->set_option( boost::asio::socket_base::keep_alive( m_tcp_socket_option.m_keep_alive ) );
			connect_info->m_socket->set_option( boost::asio::socket_base::enable_connection_aborted( m_tcp_socket_option.m_enable_connection_aborted ) );
			connect_info->m_socket->set_option( boost::asio::socket_base::receive_buffer_size( m_tcp_socket_option.m_recv_buffer_size ) );
			connect_info->m_socket->set_option( boost::asio::socket_base::send_buffer_size( m_tcp_socket_option.m_send_buffer_size ) );

			connect_info->m_available = true;
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

			connect_info->m_condition.notify_one(); //

			FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� �������ӣ�[{0}]->[{1}]��{2}", connect_info->m_endpoint_l, connect_info->m_endpoint_r, connect_info->m_node_type );
			LogPrint( syslog_level::c_info, m_log_cate, log_info );
		}
		else {
			// ��ʰ�����ڳ�ʱ����
			FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� ���� ���� ����{0}", error.message().c_str() );
			LogPrint( syslog_level::c_error, m_log_cate, log_info );
		}
	}

	int32_t AsioClient_P::Client_ReadData_Sy( ConnectAsio* connect_info, char* data, int32_t size ) {
		if( connect_info != nullptr && connect_info->m_available != false ) {
			try {
				return (int32_t)( boost::asio::read( *(connect_info->m_socket), boost::asio::buffer( data, size ) ) );
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Client_CloseOnError( connect_info );
				FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� ���� Data ��ȡ �쳣��{0}", ex.what() );
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				return -1;
			}
		}

		return 0;
	}

	int32_t AsioClient_P::Client_SendData_Sy( ConnectAsio* connect_info, const char* data, int32_t size ) {
		if( connect_info != nullptr && connect_info->m_available != false ) {
			try {
				return (int32_t)( boost::asio::write( *(connect_info->m_socket), boost::asio::buffer( data, size ) ) );
			}
			catch( std::exception& ex ) {
				std::string log_info;
				Client_CloseOnError( connect_info );
				FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� ���� Data ���� �쳣��{0}", ex.what() );
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
				return -1;
			}
		}

		return 0;
	}

	void AsioClient_P::Client_CloseByUser( ConnectAsio* connect_info ) {
		if( connect_info ) {
			connect_info->m_active_close = true;
			Client_CloseOnError( connect_info );
		}
	}

	void AsioClient_P::Client_CloseOnError( ConnectAsio* connect_info ) {
		if( connect_info ) {
			connect_info->m_available = false;

			if( true == connect_info->m_active_close && connect_info->m_socket.get() ) { // �����ر�
				connect_info->m_socket->shutdown( boost::asio::ip::tcp::socket::shutdown_both, boost::system::error_code() );
			}

			if( connect_info->m_socket.get() ) {
				connect_info->m_socket->close();
			}

			std::string log_info;
			if( true == connect_info->m_active_close ) { // �����ر�
				FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� ���� �ر����ӡ�[{0}]->[{1}]", connect_info->m_endpoint_l, connect_info->m_endpoint_r );
			}
			else { // �����ر�
				FormatLibrary::StandardLibrary::FormatTo( log_info, "�ͻ��� ���� �ر����ӡ�[{0}]->[{1}]", connect_info->m_endpoint_l, connect_info->m_endpoint_r );
			}
			LogPrint( syslog_level::c_warn, m_log_cate, log_info );

			//delete connect_info; // Ŀǰ������ɾ��
			m_disconnect_info_lock.lock();
			m_list_disconnect_info.push_back( connect_info );
			m_disconnect_info_lock.unlock();

			connect_info = nullptr;
		}
	}

	AsioClient::AsioClient()
		: m_asio_client_p( nullptr ) {
		try {
			m_asio_client_p = new AsioClient_P();
		}
		catch( ... ) {}
	}

	AsioClient::~AsioClient() {
		if( m_asio_client_p != nullptr ) {
			delete m_asio_client_p;
			m_asio_client_p = nullptr;
		}
	}

	void AsioClient::StartNetwork() {
		m_asio_client_p->StartNetwork();
	}

	bool AsioClient::IsNetworkStarted() {
		return m_asio_client_p->IsNetworkStarted();
	}

	ConnectAsio* AsioClient::Client_AddConnect( std::string address, int32_t port, std::string node_type, int32_t time_out_sec ) {
		return m_asio_client_p->Client_AddConnect( address, port, node_type, time_out_sec );
	}

	int32_t AsioClient::Client_ReadData_Sy( ConnectAsio* connect_info, char* data, int32_t size ) {
		return m_asio_client_p->Client_ReadData_Sy( connect_info, data, size );
	}

	int32_t AsioClient::Client_SendData_Sy( ConnectAsio* connect_info, const char* data, int32_t size ) {
		return m_asio_client_p->Client_SendData_Sy( connect_info, data, size );
	}

	void AsioClient::Client_CloseByUser( ConnectAsio* connect_info ) {
		m_asio_client_p->Client_CloseByUser( connect_info );
	}

} // namespace basicx
