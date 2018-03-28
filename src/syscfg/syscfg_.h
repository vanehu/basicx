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

#ifndef BASICX_SYSCFG_SYSCFG_P_H
#define BASICX_SYSCFG_SYSCFG_P_H

#include <mutex>

#include "syscfg.h"

namespace basicx {

	class SysCfg_P
	{
	public:
		SysCfg_P();
		~SysCfg_P();

	public:
		std::string m_name_app_full;
		std::string m_path_app_exec;
		std::string m_path_app_folder;
		std::string m_path_cfg_folder;
		std::string m_path_plu_folder;
		std::string m_path_ext_folder;
		std::string m_path_cfg_basic;

		CfgBasic m_cfg_basic;

		std::mutex m_holiday_lock;
		std::vector<int32_t> m_vec_holiday;
	};

} // namespace basicx

#endif // BASICX_SYSCFG_SYSCFG_P_H
