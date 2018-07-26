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

#ifndef BASICX_SYSRTM_SYSRTM_P_H
#define BASICX_SYSRTM_SYSRTM_P_H

#include <thread>

#include <network/server.h>

#include "sysrtm.h"

namespace basicx {

	typedef std::shared_ptr<NetServer> NetServerPtr;

	class SysRtm_P : public NetServer_X
	{
	public:
		SysRtm_P();
		~SysRtm_P();

	public:
		void OnNetServerInfo( NetServerInfo& net_server_info );
		void OnNetServerData( NetServerData& net_server_data );

		void StartNetServer();
		// 0：调试(debug)，1：信息(info)，2：提示(hint)，3：警告(warn)，4：错误(error)，5：致命(fatal)
		void LogTrans( syslog_level log_level, std::string& log_cate, std::string& log_info );

	public:
		NetServerPtr m_net_server_broad;

	private:
		SysCfg_S* m_syscfg;
		SysLog_S* m_syslog;
	};

} // namespace basicx

#endif // BASICX_SYSRTM_SYSRTM_P_H
