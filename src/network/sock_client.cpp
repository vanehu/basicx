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

#include <thread>

#include <common/sysdef.h>

#ifdef __OS_WINDOWS__
#include <winsock2.h>
#include <WS2tcpip.h> // inet_pton、InetPton
#endif

#include "sock_client_.h"

namespace basicx {

	SockClient_P::SockClient_P()
		: m_time_out( 10 )
		, m_connect_ok( false )
		, m_socket( -1 ) {
	}

	SockClient_P::~SockClient_P() {
		Close();
	}

	int32_t SockClient_P::Connect( std::string address, int32_t port, int32_t try_times, int32_t span_time/* = 1000*/ ) { // try_times 为 0 时无限重连 // span_time 单位毫秒
#ifdef __OS_WINDOWS__
		if( INVALID_SOCKET == m_socket ) {
			m_socket = (int32_t)( socket( AF_INET, SOCK_STREAM, 0 ) );
			if( INVALID_SOCKET == m_socket ) {
				return -1;
			}
		}

		SOCKADDR_IN	sock_addr_r;
		memset( &sock_addr_r, 0, sizeof( sock_addr_r ) );
		sock_addr_r.sin_family = AF_INET;
		//sock_addr_r.sin_addr.s_addr = inet_addr( address.c_str() );
		inet_pton( AF_INET, address.c_str(), (void*)&sock_addr_r.sin_addr ); // InetPton
		sock_addr_r.sin_port = htons( port );

		int32_t try_times_count = 0;
		while( false == m_connect_ok ) {
			if( 0 == connect( m_socket, (sockaddr*)&sock_addr_r, sizeof( sock_addr_r ) ) ) {
				m_connect_ok = true;
				break;
			}
			else {
				std::this_thread::sleep_for( std::chrono::milliseconds( span_time ) );
			}

			if( try_times > 0 ) {
				try_times_count++;
				if( try_times_count >= try_times ) {
					return -2;
				}
			}
		}

		return 0;
#else
		return -1;
#endif
	}

	bool SockClient_P::IsConnectValid() {
#ifdef __OS_WINDOWS__
		if( INVALID_SOCKET == m_socket || false == m_connect_ok ) {
			return false;
		}

		timeval	time_out;
		time_out.tv_sec = 0;
		time_out.tv_usec = 0;
		fd_set my_fd_set;
		FD_ZERO( &my_fd_set );
		FD_SET( m_socket, &my_fd_set );

		if( SOCKET_ERROR == select( m_socket, &my_fd_set, nullptr, nullptr, &time_out ) ) {
			return false;
		}

		return true;
#else
		return false;
#endif
	}

	int32_t SockClient_P::Read( void* buffer, int32_t length ) {
#ifdef __OS_WINDOWS__
		if( INVALID_SOCKET == m_socket || false == m_connect_ok ) {
			return -1;
		}

		int32_t recv_length = 0;
		int32_t read_length = 0;

		fd_set my_fd_set;
		timeval	time_out;
		time_out.tv_sec = m_time_out;
		time_out.tv_usec = 0;

		while( recv_length < length ) {
			FD_ZERO( &my_fd_set );
			FD_SET( m_socket, &my_fd_set );
			if( 1 == select( m_socket, &my_fd_set, nullptr, nullptr, &time_out ) ) {
				read_length = recv( m_socket, ((char*)buffer ) + recv_length, length - recv_length, 0 );
				if( read_length <= 0 ) {
					Close();
					return -2; // 读数据出错：连接已经断开
				}
				recv_length += read_length;
			}
			else {
				Close();
				return -3; // 读数据出错：接收数据超时
			}
		}

		return recv_length;
#else
		return 0;
#endif
	}

	int32_t SockClient_P::Write( void* buffer, int32_t length ) {
#ifdef __OS_WINDOWS__
		if( INVALID_SOCKET == m_socket || false == m_connect_ok ) {
			return -1;
		}

		int32_t send_length = 0;
		int32_t write_length = 0;

		while( send_length < length ) {
			write_length = send( m_socket, ((char*)buffer ) + send_length, length - send_length, 0 );
			if( write_length <= 0 ) {
				Close();
				return -2; // 写数据出错：连接已经断开
			}
			send_length += write_length;
		}

		return send_length;
#else
		return 0;
#endif
	}

	bool SockClient_P::IsReadable( int32_t wait_time/* = 0*/ ) {
#ifdef __OS_WINDOWS__
		if( INVALID_SOCKET == m_socket || false == m_connect_ok ) {
			return false;
		}

		timeval	time_out;
		time_out.tv_sec = 0;
		time_out.tv_usec = wait_time;
		fd_set my_fd_set;
		FD_ZERO( &my_fd_set );
		FD_SET( m_socket, &my_fd_set );

		if( 1 == select( m_socket, &my_fd_set, nullptr, nullptr, &time_out ) ) {
			return true;
		}

		return false;
#else
		return false;
#endif
	}

	bool SockClient_P::IsWritable( int32_t wait_time/* = 0*/ ) {
#ifdef __OS_WINDOWS__
		if( INVALID_SOCKET == m_socket || false == m_connect_ok ) {
			return false;
		}

		timeval	time_out;
		time_out.tv_sec = 0;
		time_out.tv_usec = wait_time;
		fd_set my_fd_set;
		FD_ZERO( &my_fd_set );
		FD_SET( m_socket, &my_fd_set );

		if( 1 == select( m_socket, nullptr, &my_fd_set, nullptr, &time_out ) ) {
			return true;
		}

		return false;
#else
		return false;
#endif
	}

	void SockClient_P::Close() {
#ifdef __OS_WINDOWS__
		m_connect_ok = false;
		if( m_socket != INVALID_SOCKET ) {
			closesocket( m_socket );
			m_socket = (int32_t)( INVALID_SOCKET );
		}
#endif
	}

	SockClient::SockClient()
		: m_sock_client_p( nullptr ) {
		try {
			m_sock_client_p = new SockClient_P();
		}
		catch( ... ) {}
	}

	SockClient::~SockClient() {
		if( m_sock_client_p != nullptr ) {
			delete m_sock_client_p;
			m_sock_client_p = nullptr;
		}
	}

	int32_t SockClient::Connect( std::string address, int32_t port, int32_t try_times, int32_t span_time/* = 1000*/ ) {
		return m_sock_client_p->Connect( address, port, try_times, span_time );
	}

	bool SockClient::IsConnectValid() {
		return m_sock_client_p->IsConnectValid();
	}

	int32_t SockClient::Read( void* buffer, int32_t length ) {
		return m_sock_client_p->Read( buffer, length );
	}

	int32_t SockClient::Write( void* buffer, int32_t length ) {
		return m_sock_client_p->Write( buffer, length );
	}

	bool SockClient::IsReadable( int32_t wait_time/* = 0*/ ) {
		return m_sock_client_p->IsReadable( wait_time );
	}

	bool SockClient::IsWritable( int32_t wait_time/* = 0*/ ) {
		return m_sock_client_p->IsWritable( wait_time );
	}

	void SockClient::Close() {
		m_sock_client_p->Close();
	}

} // namespace basicx
