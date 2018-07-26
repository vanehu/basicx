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

#include <iostream>

#include <common/sysdef.h>

#ifdef __OS_WINDOWS__
#include <windows.h>
#endif

#include "timers_.h"

namespace basicx {

	TimerSpace::TimerSpace( int32_t id, std::chrono::duration<int64_t> duration, double delay_s, int32_t times, std::function<void( int32_t, int64_t )> callback )
		: m_need( true )
		, m_id( id )
		, m_duration( duration )
		, m_delay_s( delay_s )
		, m_times( times )
		, m_runned( 0 )
		, m_callback( callback ) {
	}

	TimerSpace::~TimerSpace() {
	}

	TimerPoint::TimerPoint( int32_t id, std::chrono::duration<int64_t> duration, time_t datetime, int32_t times, std::function<void( int32_t, int64_t )> callback )
		: m_need( true )
		, m_id( id )
		, m_duration( duration )
		, m_datetime( datetime )
		, m_times( times )
		, m_runned( 0 )
		, m_callback( callback ) {
	}

	TimerPoint::~TimerPoint() {
	}

	Timers_P::Timers_P()
		: m_running( false )
		, m_count_id( 0 ) {
	}

	Timers_P::~Timers_P() {
	}

	void Timers_P::Start() {
		m_running = true;
		m_handler_space = std::thread( &Timers_P::HandleTimerSpace, this );
		m_handler_point = std::thread( &Timers_P::HandleTimerPoint, this );
	}

	void Timers_P::Stop() {
		m_running = false;
		m_handler_space.join();
		m_handler_point.join();
	}

	int32_t Timers_P::AddTimerSpace( std::chrono::duration<int64_t> duration, double delay_s, int32_t times, std::function<void( int32_t, int64_t )> callback ) {
		std::chrono::steady_clock::time_point time_current_steady = std::chrono::steady_clock::now();
		m_count_id++;
		TimerSpace* timer_space = new TimerSpace( m_count_id, duration, delay_s, times, callback );
		timer_space->m_create = time_current_steady;
		timer_space->m_launch = time_current_steady + std::chrono::duration<int64_t, std::ratio<1, 1000000000>>( (int64_t)( timer_space->m_delay_s * 1000000000 ) ); // �Ե�һ�δﵽʱ���Ϊ��׼
		timer_space->m_arrive = timer_space->m_launch;
		m_queue_space_timers_lock.lock();
		m_queue_space_timers.push( timer_space );
		// ��ʱΪ����Ŀ��ʱ��㣬���ÿۼ�������ʱ
		m_queue_space_timers_lock.unlock();
		m_map_space_timers_lock.lock();
		m_map_space_timers.insert( std::make_pair( m_count_id, timer_space ) );
		m_map_space_timers_lock.unlock();
		return m_count_id;
	}

	int32_t Timers_P::AddTimerPoint( std::chrono::duration<int64_t> duration, time_t datetime, int32_t times, std::function<void( int32_t, int64_t )> callback ) {
		std::chrono::system_clock::time_point time_current_system = std::chrono::system_clock::now();
		m_count_id++;
		TimerPoint* timer_point = new TimerPoint( m_count_id, duration, datetime, times, callback );
		timer_point->m_create = time_current_system;
		timer_point->m_launch = std::chrono::system_clock::from_time_t( timer_point->m_datetime ); // �Ե�һ�δﵽʱ���Ϊ��׼
		timer_point->m_arrive = timer_point->m_launch;
		m_queue_point_timers_lock.lock();
		m_queue_point_timers.push( timer_point );
		// ��ʱΪ����Ŀ��ʱ��㣬���ÿۼ�������ʱ
		m_queue_point_timers_lock.unlock();
		m_map_point_timers_lock.lock();
		m_map_point_timers.insert( std::make_pair( m_count_id, timer_point ) );
		m_map_point_timers_lock.unlock();
		return m_count_id;
	}

	void Timers_P::DelTimerSpace( int32_t id ) {
		m_map_space_timers_lock.lock();
		std::unordered_map<int32_t, TimerSpace*>::iterator it_ts = m_map_space_timers.find( id );
		if( it_ts != m_map_space_timers.end() ) {
			TimerSpace* timer_space = it_ts->second;
			timer_space->m_need = false; // ��ǲ��ٴ���
			m_map_space_timers.erase( id );
		}
		m_map_space_timers_lock.unlock();
	}

	void Timers_P::DelTimerPoint( int32_t id ) {
		m_map_point_timers_lock.lock();
		std::unordered_map<int32_t, TimerPoint*>::iterator it_ts = m_map_point_timers.find( id );
		if( it_ts != m_map_point_timers.end() ) {
			TimerPoint* timer_point = it_ts->second;
			timer_point->m_need = false; // ��ǲ��ٴ���
			m_map_point_timers.erase( id );
		}
		m_map_point_timers_lock.unlock();
	}

#ifdef __OS_WINDOWS__
	int64_t g_handler_wait_time_def = -5000; // 0.5ms����λ 100 ���� // ���� steady_clock ���ƣ����� system_clock ����
	LARGE_INTEGER g_handler_wait_time_set = { (unsigned long)( g_handler_wait_time_def & 0xFFFFFFFF ), (long)( g_handler_wait_time_def >> 32 ) };
	void __stdcall TimerCallFunc( void* data_value, unsigned long timer_low_value, unsigned long timer_high_value ) {
	}
#endif

	void Timers_P::HandleTimerSpace() {
		while( true == m_running ) {
#ifdef __OS_WINDOWS__
			void* waitable_timer = CreateWaitableTimer( NULL, FALSE, TEXT( "space" ) );
			if( waitable_timer != nullptr ) {
				bool success = SetWaitableTimer( waitable_timer, &g_handler_wait_time_set, 0, TimerCallFunc, NULL, FALSE );
			}
#endif
			while( !m_queue_space_timers.empty() ) {
				m_queue_space_timers_lock.lock();
				TimerSpace* timer_space = m_queue_space_timers.top();
				if( timer_space->m_need != true ) {
					m_queue_space_timers.pop();
					delete timer_space;
				}
				else {
					std::chrono::steady_clock::time_point time_current_steady = std::chrono::steady_clock::now();
					if( time_current_steady >= timer_space->m_arrive ) { // �ѵ���
						timer_space->m_callback( timer_space->m_id, std::chrono::duration_cast<std::chrono::nanoseconds>( time_current_steady - timer_space->m_arrive ).count() ); // ���ûص�����
						timer_space->m_runned++; // ��һ��
						m_queue_space_timers.pop();
						if( 0 == timer_space->m_times || timer_space->m_runned < timer_space->m_times ) {
							timer_space->m_create = time_current_steady; //
							timer_space->m_arrive = timer_space->m_launch + timer_space->m_duration * timer_space->m_runned;
							m_queue_space_timers.push( timer_space );
							// ��ʱΪ����Ŀ��ʱ��㣬���ÿۼ������ͻص���ʱ
						}
						else { // ȫ�����
							m_map_space_timers_lock.lock();
							m_map_space_timers.erase( timer_space->m_id );
							m_map_space_timers_lock.unlock();
							delete timer_space;
						}
					}
					else {
						m_queue_space_timers_lock.unlock(); //
						break; // ʣ�µĶ�û�е���
					}
				}
				m_queue_space_timers_lock.unlock();
			}
			// ȥ�� sleep ���Դﵽ���� 10΢�� ���µľ��ȣ�����ռ���൱��һ���˵� CPU ��Դ
			// ���� SetTimer.exe ʱ��ϵͳĬ�� 10ms ���ȣ���������������С 0.5ms �� sleep �ľ����� 1.6ms ����
			// ʹ�� BindProcess() �� SetThreadPriority() û���������ã�������ȥ�� sleep ʱ������ʱ��������
			// GetPerformanceFrequency() �� GetPerformanceTickCount() ������Ϊ CPU �ܺı�Ƶ���ı�
#ifdef __OS_WINDOWS__
			SleepEx( INFINITE, TRUE ); // ò�ƾ��ȱ� sleep_for ��΢��һЩ
#else
			std::this_thread::sleep_for( std::chrono::nanoseconds( 1 ) );
#endif
		}
	}

	void Timers_P::HandleTimerPoint() {
		while( true == m_running ) {
#ifdef __OS_WINDOWS__
			void* waitable_timer = CreateWaitableTimer( NULL, FALSE, TEXT( "point" ) );
			if( waitable_timer != nullptr ) {
				bool success = SetWaitableTimer( waitable_timer, &g_handler_wait_time_set, 0, TimerCallFunc, NULL, FALSE );
			}
#endif
			while( !m_queue_point_timers.empty() ) {
				m_queue_point_timers_lock.lock();
				TimerPoint* timer_point = m_queue_point_timers.top();
				if( timer_point->m_need != true ) {
					m_queue_point_timers.pop();
					delete timer_point;
				}
				else {
					std::chrono::system_clock::time_point time_current_system = std::chrono::system_clock::now();
					if( time_current_system >= timer_point->m_arrive ) { // �ѵ���
						timer_point->m_callback( timer_point->m_id, std::chrono::duration_cast<std::chrono::nanoseconds>( time_current_system - timer_point->m_arrive ).count() ); // ���ûص�����
						timer_point->m_runned++; // ��һ��
						m_queue_point_timers.pop();
						if( 0 == timer_point->m_times || timer_point->m_runned < timer_point->m_times ) {
							timer_point->m_create = time_current_system; //
							timer_point->m_arrive = timer_point->m_launch + timer_point->m_duration * timer_point->m_runned;
							m_queue_point_timers.push( timer_point );
							// ��ʱΪ����Ŀ��ʱ��㣬���ÿۼ������ͻص���ʱ
						}
						else { // ȫ�����
							m_map_point_timers_lock.lock();
							m_map_point_timers.erase( timer_point->m_id );
							m_map_point_timers_lock.unlock();
							delete timer_point;
						}
					}
					else {
						m_queue_point_timers_lock.unlock(); //
						break; // ʣ�µĶ�û�е���
					}
				}
				m_queue_point_timers_lock.unlock();
			}
			// ȥ�� sleep ���Դﵽ���� 10΢�� ���µľ��ȣ�����ռ���൱��һ���˵� CPU ��Դ
			// ���� SetTimer.exe ʱ��ϵͳĬ�� 10ms ���ȣ���������������С 0.5ms �� sleep �ľ����� 1.6ms ����
			// ʹ�� BindProcess() �� SetThreadPriority() û���������ã�������ȥ�� sleep ʱ������ʱ��������
			// GetPerformanceFrequency() �� GetPerformanceTickCount() ������Ϊ CPU �ܺı�Ƶ���ı�
#ifdef __OS_WINDOWS__
			SleepEx( INFINITE, TRUE ); // ò�ƾ��ȱ� sleep_for ��΢��һЩ
#else
			std::this_thread::sleep_for( std::chrono::nanoseconds( 1 ) );
#endif
		}
	}

	void Timers_P::Test_TimerCallBack( int32_t timer_id, int64_t delay_ns ) {
		std::cout << "m - timer: " << timer_id << " ��ʱ " << (double)delay_ns / 1000000.0 << " ����\n";
	}

	Timers_K::Timers_K()
		: m_timers_p( nullptr ) {
			try {
				m_timers_p = new Timers_P();
			}
			catch( ... ) {}
	}

	Timers_K::~Timers_K() {
		if( m_timers_p != nullptr ) {
			delete m_timers_p;
			m_timers_p = nullptr;
		}
	}

	Timers_D::Timers_D()
		: m_timers_p( nullptr ) {
		try {
			m_timers_p = new Timers_P();
		}
		catch( ... ) {}
	}

	Timers_D::~Timers_D() {
		if( m_timers_p != nullptr ) {
			delete m_timers_p;
			m_timers_p = nullptr;
		}
	}

	void Timers_D::Start() {
		m_timers_p->Start();
	}

	void Timers_D::Stop() {
		m_timers_p->Stop();
	}

	int32_t Timers_D::AddTimerSpace( std::chrono::duration<int64_t> duration, double delay_s, int32_t times, std::function<void( int32_t, int64_t )> callback ) {
		return m_timers_p->AddTimerSpace( duration, delay_s, times, callback );
	}

	int32_t Timers_D::AddTimerPoint( std::chrono::duration<int64_t> duration, time_t datetime, int32_t times, std::function<void( int32_t, int64_t )> callback ) {
		return m_timers_p->AddTimerPoint( duration, datetime, times, callback );
	}

	void Timers_D::DelTimerSpace( int32_t id ) {
		m_timers_p->DelTimerSpace( id );
	}

	void Timers_D::DelTimerPoint( int32_t id ) {
		m_timers_p->DelTimerPoint( id );
	}

	void Timers_D::Test_TimerCallBack( int32_t timer_id, int64_t delay_ns ) {
		std::cout << "m - timer: " << timer_id << " ��ʱ " << (double)delay_ns / 1000000.0 << " ����\n";
	}

	basicx::Timers_S* basicx::Timers_S::m_instance = nullptr;

	Timers_S::Timers_S()
		: m_timers_p( nullptr ) {
		try {
			m_timers_p = new Timers_P();
		}
		catch( ... ) {}
		m_instance = this;
	}

	Timers_S::~Timers_S() {
		if( m_timers_p != nullptr ) {
			delete m_timers_p;
			m_timers_p = nullptr;
		}
	}

	Timers_S* Timers_S::GetInstance() {
		return m_instance;
	}

	void Timers_S::Start() {
		m_timers_p->Start();
	}

	void Timers_S::Stop() {
		m_timers_p->Stop();
	}

	int32_t Timers_S::AddTimerSpace( std::chrono::duration<int64_t> duration, double delay_s, int32_t times, std::function<void( int32_t, int64_t )> callback ) {
		return m_timers_p->AddTimerSpace( duration, delay_s, times, callback );
	}

	int32_t Timers_S::AddTimerPoint( std::chrono::duration<int64_t> duration, time_t datetime, int32_t times, std::function<void( int32_t, int64_t )> callback ) {
		return m_timers_p->AddTimerPoint( duration, datetime, times, callback );
	}

	void Timers_S::DelTimerSpace( int32_t id ) {
		m_timers_p->DelTimerSpace( id );
	}

	void Timers_S::DelTimerPoint( int32_t id ) {
		m_timers_p->DelTimerPoint( id );
	}

} // namespace basicx
