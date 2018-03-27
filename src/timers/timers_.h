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

#ifndef BASICX_TIMERS_TIMERS_P_H
#define BASICX_TIMERS_TIMERS_P_H

#include <queue>
#include <ctime>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <stdint.h> // int32_t, int64_t
#include <functional>
#include <unordered_map>

#include "timers.h"

namespace basicx {

	class TimerSpace
	{
	private:
		TimerSpace() {};

	public:
		TimerSpace( int32_t id, std::chrono::duration<int64_t> duration, double delay_s, int32_t times, std::function<void( int32_t, int64_t )> callback );
		~TimerSpace();

	public:
		std::atomic<bool> m_need;
		int32_t m_id;
		double m_delay_s;
		std::chrono::duration<int64_t> m_duration; // 周期
		int32_t m_times; // 0 为无限次
		int32_t m_runned; // 已运行次数
		std::chrono::steady_clock::time_point m_create;
		std::chrono::steady_clock::time_point m_launch;
		std::chrono::steady_clock::time_point m_arrive;
		std::function<void( int32_t, int64_t )> m_callback;
	};

	class TimerPoint
	{
	private:
		TimerPoint() {};

	public:
		TimerPoint( int32_t id, std::chrono::duration<int64_t> duration, time_t datetime, int32_t times, std::function<void( int32_t, int64_t )> callback );
		~TimerPoint();

	public:
		std::atomic<bool> m_need;
		int32_t m_id;
		time_t m_datetime;
		std::chrono::duration<int64_t> m_duration; // 周期
		int32_t m_times; // 0 为无限次
		int32_t m_runned; // 已运行次数
		std::chrono::system_clock::time_point m_create;
		std::chrono::system_clock::time_point m_launch;
		std::chrono::system_clock::time_point m_arrive;
		std::function<void( int32_t, int64_t )> m_callback;
	};

	struct TimerSpaceCompare
	{
		bool operator () ( TimerSpace* a, TimerSpace* b ) const { // 反向定义实现最小值优先
			return a->m_arrive > b->m_arrive;
		}
	};

	struct TimerPointCompare
	{
		bool operator () ( TimerPoint* a, TimerPoint* b ) const { // 反向定义实现最小值优先
			return a->m_arrive > b->m_arrive;
		}
	};

	class Timers_P
	{
	public:
		Timers_P();
		~Timers_P();

	public:
		void Start();
		void Stop();
		int32_t AddTimerSpace( std::chrono::duration<int64_t> duration, double delay_s, int32_t times, std::function<void( int32_t, int64_t )> callback );
		int32_t AddTimerPoint( std::chrono::duration<int64_t> duration, time_t datetime, int32_t times, std::function<void( int32_t, int64_t )> callback );
		void DelTimerSpace( int32_t id );
		void DelTimerPoint( int32_t id );
		void HandleTimerSpace();
		void HandleTimerPoint();

	public:
		void Test_TimerCallBack( int32_t timer_id, int64_t delay_ns );

	private:
		std::atomic<bool> m_running;
		std::thread m_handler_space;
		std::thread m_handler_point;
		
		int32_t m_count_id;
		std::mutex m_queue_space_timers_lock;
		std::mutex m_queue_point_timers_lock;
		std::priority_queue<TimerSpace*, std::vector<TimerSpace*>, TimerSpaceCompare> m_queue_space_timers;
		std::priority_queue<TimerPoint*, std::vector<TimerPoint*>, TimerPointCompare> m_queue_point_timers;
		std::mutex m_map_space_timers_lock;
		std::mutex m_map_point_timers_lock;
		std::unordered_map<int32_t, TimerSpace*> m_map_space_timers;
		std::unordered_map<int32_t, TimerPoint*> m_map_point_timers;
	};

} // namespace basicx

#endif // BASICX_TIMERS_TIMERS_P_H
