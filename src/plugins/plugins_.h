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

#ifndef BASICX_PLUGINS_PLUGINS_P_H
#define BASICX_PLUGINS_PLUGINS_P_H

#include <syslog/syslog.h>

#include "plugin_info.h"

namespace basicx {

#pragma pack( push )
#pragma pack( 1 )

	struct PluginItem
	{
		std::string m_name;
		std::string m_state;
		std::string m_version;
		std::string m_category;
		std::string m_description;
		std::string m_dependence;
	};

#pragma pack( pop )

	class Plugins_P
	{
	public:
		Plugins_P();
		~Plugins_P();

	public:
		void LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show = 0 );

		void StartPlugins();
		bool IsPluginsStarted();

	public:
		std::vector<std::string> GetErrorInfos() const;
		bool AddErrorInfo( const std::string& error_info );

		void SetInfoFileExt( const std::string& info_file_ext );
		std::string GetInfoFileExt() const;
		void SetPluginFolder( const std::string& plugin_folder );
		std::string GetPluginFolder() const;

		void SetPluginInfoPaths( const std::string& plugin_folder );
		void FindPluginPaths( std::string folder_path );
		std::vector<std::string> GetPluginInfoPaths() const;
		bool LoadPluginSetting();
		std::vector<std::string> GetDisabledPlugins() const;
		std::vector<std::string> GetForceEnabledPlugins() const;
		bool WritePluginSetting();

		void LoadPluginInfos();
		void ResolveDepends();
		std::vector<PluginInfo*> LoadQueue();
		bool LoadQueue( PluginInfo* plugin_info, std::vector<PluginInfo*>& queue, std::vector<PluginInfo*>& cycle_check_queue );
		std::vector<PluginInfo*> GetPluginInfos() const;
		size_t GetPluginInfosNumber();
		PluginInfo* GetPluginInfo( size_t index );
		PluginInfo* FindPluginInfoByName( const std::string& plugin_name );

		bool LoadAll( std::string folder );
		void LoadPlugins();
		void LoadPlugin( PluginInfo* plugin_info, PluginInfo::State dest_state );
		void StopAll();

		Plugins_X* GetPluginsX( const std::string& plugin_name );
		void SetPluginsX( const std::string& plugin_name, Plugins_X* plugins_x );

		std::string GetPluginLocationByName( const std::string& plugin_name );
		std::string GetPluginCfgFilePathByName( const std::string& plugin_name );
		std::string GetPluginInfoFilePathByName( const std::string& plugin_name );

		//void AddUserTaskSend( int32_t task_type, std::string& task_info, std::string& task_data = std::string( "" ) );
		void OnDeliverTask( int32_t task_id, std::string node_type, int32_t identity, int32_t code, std::string& data );
		void CommitResult( int32_t task_id, int32_t identity, int32_t code, std::string& data );

	public:
		std::string m_info_file_ext;
		std::string m_plugin_folder;
		std::vector<PluginInfo*> m_vec_plugin_info;
		std::vector<std::string> m_vec_plugin_info_path;
		std::vector<std::string> m_vec_disabled_plugin;
		std::vector<std::string> m_vec_force_enabled_plugin;
		std::vector<std::string> m_vec_error_info_list;

	public:
		bool m_plugins_running;
		std::vector<Plugins_X*> m_vec_plugins_x; //xrd//

	private:
		SysLog_S* m_syslog;
		std::string m_log_cate;
		//Mission* m_mission; //mission//
	};

} // namespace basicx

#endif // BASICX_PLUGINS_PLUGINS_P_H
