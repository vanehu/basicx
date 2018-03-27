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

#ifndef BASICX_TIMERS_TESTER_H
#define BASICX_TIMERS_TESTER_H

#include <thread>
#include <iostream>

#include "timers.h"

namespace basicx {

	void Test_TimerCallBack( int32_t timer_id, int64_t delay_ns ) {
		switch( timer_id ) {
		case 1: { // 将导致后面 2 回调延时
			std::cout << "g - timer " << timer_id << " 延时 " << (double)delay_ns / 1000000.0 << " 毫秒\n";
			break;
		}
		case 2: { // 延时受前面 1 回调影响
			std::cout << "g - timer " << timer_id << " 延时 " << (double)delay_ns / 1000000.0 << " 毫秒\n";
			break;
		}
		case 3: { // 将导致后面 4 回调延时
			std::cout << "g - timer " << timer_id << " 延时 " << (double)delay_ns / 1000000.0 << " 毫秒\n";
			break;
		}
		case 4: { // 延时受前面 3 回调影响
			std::cout << "g - timer " << timer_id << " 延时 " << (double)delay_ns / 1000000.0 << " 毫秒\n";
			break;
		}
		default:
			break;
		}
	}

	void Test_Timers() {
		Timers_D timers_d;
		timers_d.Start();

		int32_t times = 10;
		double delay_s = 1.0;
		std::chrono::duration<int64_t> duration_space = std::chrono::seconds( 1 );
		std::chrono::duration<int64_t> duration_point = std::chrono::duration<int64_t, std::ratio<1, 1>>( 1 );

		int32_t timer_1 = timers_d.AddTimerSpace( duration_space, delay_s, times, Test_TimerCallBack );
		int32_t timer_2 = timers_d.AddTimerSpace( duration_space, delay_s, times, Test_TimerCallBack );

		time_t time_t_3 = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() + std::chrono::seconds( 1 ) );
		int32_t timer_3 = timers_d.AddTimerPoint( duration_point, time_t_3, times, std::bind( &Timers_D::Test_TimerCallBack, &timers_d, std::placeholders::_1, std::placeholders::_2 ) );

		time_t time_t_4 = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() + std::chrono::seconds( 1 ) );
		int32_t timer_4 = timers_d.AddTimerPoint( duration_point, time_t_4, times, std::bind( &Timers_D::Test_TimerCallBack, &timers_d, std::placeholders::_1, std::placeholders::_2 ) );

		std::this_thread::sleep_for( std::chrono::seconds( 5 ) );

		timers_d.DelTimerSpace( timer_1 );
		timers_d.DelTimerPoint( timer_3 );

		std::this_thread::sleep_for( std::chrono::seconds( 30 ) );

		timers_d.Stop();
	}

} // namespace basicx

#endif // BASICX_TIMERS_TESTER_H
