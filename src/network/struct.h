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

#ifndef BASICX_NETWORK_STRUCT_H
#define BASICX_NETWORK_STRUCT_H

#include <map>
#include <list>
#include <mutex>
#include <time.h>
#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <stdint.h>
#include <unordered_map>

#include <boost/asio.hpp> // 在 #include <windows.h> 前包含，不然会报：WinSock.h has already been included
#include <boost/bind.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <common/define.h>
#include <common/Format/Format.hpp>
#include <syslog/syslog.h>

namespace basicx {

	#define TYPE_BYTES 1 // 0~F：0~15
	#define CODE_BYTES 1 // 0~F：0~15
	#define SIZE_BYTES 6 // 000000~FFFFFF：0~16777215：15MB
    #define FLAG_BYTES 2 // TYPE_BYTES + CODE_BYTES
    #define HEAD_BYTES 8 // TYPE_BYTES + CODE_BYTES + SIZE_BYTES

	//typedef boost::shared_ptr<boost::thread> ThreadPtr;
	typedef std::shared_ptr<std::thread> ThreadPtr;
	typedef std::shared_ptr<boost::asio::io_service> ServicePtr;
	typedef std::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
	typedef std::shared_ptr<boost::asio::ip::tcp::acceptor> AcceptorPtr;
	typedef std::shared_ptr<boost::asio::deadline_timer> TimerPtr;

	#pragma pack( push )
	#pragma pack( 1 )

	struct ReconnectInfo
	{
		int32_t m_identity;
		std::string m_address;
		int32_t m_port;
		std::string m_node_type;
		int32_t m_reconnect_count;
	};

	struct ListenInfo
	{
		std::string m_address;
		int32_t m_port;
		std::string m_node_type;
		int32_t m_connect_number;
		AcceptorPtr m_acceptor;
	};

	struct ServerInfo
	{
		std::string m_address;
		int32_t m_port;
		std::string m_node_type;
		int32_t m_connect_number;
	};

	struct ConnectInfo
	{
		SocketPtr m_socket;
		TimerPtr m_connect_timer;
		int32_t m_identity; // 连接唯一标记
		std::atomic<bool> m_available; // 标记连接是否成功
		time_t m_heart_check_time; // 在有发送数据(含心跳检测)时才更新，为减少连接查找开销，接收数据时不更新
		std::atomic<bool> m_active_close; // 标记主动或被动关闭

		std::string m_endpoint_r; // 连接标记如"tcp://192.16.1.23:333"
		std::string m_protocol_r;
		std::string m_adress_r;
		int32_t m_port_r;
		std::string m_str_port_r;

		std::string m_endpoint_l; // 连接标记如"tcp://192.16.1.38:465"
		std::string m_protocol_l;
		std::string m_adress_l;
		int32_t m_port_l;
		std::string m_str_port_l;

		std::string m_node_type; // 本地监听或远程连接节点类型

		char* m_recv_buf_head;
		char* m_recv_buf_data;
		size_t m_recv_buf_data_size;

		int32_t m_stat_lost_msg;

		void clear() {
			if( m_recv_buf_head != nullptr ) {
				delete[] m_recv_buf_head;
				m_recv_buf_head = nullptr;
			}
			if( m_recv_buf_data != nullptr ) {
				delete[] m_recv_buf_data;
				m_recv_buf_data = nullptr;
			}
		}
	};

	struct SendBufInfo
	{
		size_t m_send_size;
		std::string m_send_buf;
		ConnectInfo* m_connect_info;

		SendBufInfo()
			: m_connect_info( nullptr )
			, m_send_size( 0 ) {
		}

		SendBufInfo( ConnectInfo* connect_info, size_t send_size, std::ostringstream& type, std::ostringstream& code, std::ostringstream& size, std::string& data )
			: m_connect_info( connect_info )
			, m_send_size( send_size ) {
			m_send_buf.append( type.str() );
			m_send_buf.append( code.str() );
			m_send_buf.append( size.str() );
			m_send_buf.append( data );
		}

		void Update( ConnectInfo* connect_info, size_t send_size, std::ostringstream& type, std::ostringstream& code, std::ostringstream& size, std::string& data ) {
			m_connect_info = connect_info;
			m_send_size = send_size;
			m_send_buf.clear(); //
			m_send_buf.append( type.str() );
			m_send_buf.append( code.str() );
			m_send_buf.append( size.str() );
			m_send_buf.append( data );
		}
	};

	class SenderVector
	{
	private:
		SenderVector() {};

	public:
		SenderVector( size_t capacity )
			: m_count( 0 )
			, m_handled( 0 ) {
			if( 0 == capacity ) { // 不限缓存数量，则默认初始化 8192 个
				m_capacity = 8192;
			}
			else { // 限制缓存数量
				m_capacity = capacity;
			}
			for( size_t i = 0; i < m_capacity; i++ ) {
				m_vec_send_buf_info.push_back( new SendBufInfo() );
			}
		}

		~SenderVector() {
			for( size_t i = 0; i < m_vec_send_buf_info.size(); i++ ) {
				if( m_vec_send_buf_info[i] != nullptr ) {
					delete m_vec_send_buf_info[i];
				}
			}
			m_vec_send_buf_info.clear();
		}

	public:
		std::atomic<size_t> m_count;
		std::atomic<size_t> m_handled;
		std::atomic<size_t> m_capacity;
		std::vector<SendBufInfo*> m_vec_send_buf_info;
	};

	struct TCPSocketOption
	{
		bool m_debug;
		bool m_no_delay;
		bool m_keep_alive;
		bool m_enable_connection_aborted;
		bool m_reuse_address;
		int32_t m_recv_buffer_size;
		int32_t m_send_buffer_size;

		TCPSocketOption()
			: m_debug( false )
			, m_no_delay( true )
			, m_keep_alive( true )
			, m_enable_connection_aborted( true )
			, m_reuse_address( true )
			, m_recv_buffer_size( 61320 ) // 1460(MSS) * 42 = 61320
			, m_send_buffer_size( 61320 ) { // 1460(MSS) * 42 = 61320
		}
	};

	#pragma pack( pop )

} // namespace basicx

#endif // BASICX_NETWORK_STRUCT_H
