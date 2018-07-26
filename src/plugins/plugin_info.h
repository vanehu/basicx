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

#ifndef BASICX_PLUGINS_PLUGIN_INFO_H
#define BASICX_PLUGINS_PLUGIN_INFO_H

#include <vector>

#include <common/sysdef.h>
#include <syslog/syslog.h>

#include "plugins.h"

namespace basicx {

#pragma pack( push )
#pragma pack( 1 )

	struct PluginDepend
	{
		std::string m_name;
		std::string m_version;
		bool operator == ( const PluginDepend& other ) {
			return m_name == other.m_name && m_version == other.m_version;
		}
	};

	struct PluginArgument
	{
		std::string m_name;
		std::string m_parameter;
		std::string m_description;
		bool operator == ( const PluginArgument& other ) { // 不比较描述，只比较参数名和参数值
			return m_name == other.m_name && m_parameter == other.m_parameter;
		}
	};

#pragma pack( pop )

	class PluginInfo
	{
	public:
		enum class State {
			invalid = 0, readed = 1, resolved = 2, loaded = 3, initialized = 4, running = 5, stopped = 6, deleted = 7,
		};

	public:
		PluginInfo();
		~PluginInfo();

	public:
		void LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show = 0 );

	public:
		State GetState() const;
		void SetEnabled( bool value );
		void SetDisabledIndirectly( bool value );
		void SetExperimental( bool value );
		void SetError( bool value );
		bool IsEnabled() const;
		bool IsDisabledIndirectly() const;
		bool IsExperimental() const;
		bool HasError() const;
		void AddErrorInfo( const std::string& error_info ); // has_error = true;
		std::vector<std::string> GetErrorInfos() const;

		bool ReadPluginInfo( const std::string& info_file_path );
		bool ReadPluginInfoFromXML( std::string info_file_path );
		bool IsValidVersion( const std::string& version, const std::string& plugin_name );
		std::string Name() const;
		std::string Version() const;
		std::string CompatVersion() const;
		std::string Vendor() const;
		std::string Copyright() const;
		std::string License() const;
		std::string Category() const;
		std::string Description() const;
		std::string Url() const;
		void SetLocation( std::string path_folder );
		void SetInfoFilePath( std::string path_info_file );
		void SetCfgFilePath( std::string path_cfg_file );
		std::string GetLocation() const;
		std::string GetInfoFilePath() const;
		std::string GetCfgFilePath() const;

		std::vector<PluginDepend> GetDepends() const;
		size_t GetDependsNumber();
		std::string GetDependName( size_t index );
		std::string GetDependVersion( size_t index );
		std::vector<PluginArgument> GetArguments() const;
		size_t GetArgumentsNumber();
		std::string GetArgumentName( size_t index );
		std::string GetArgumentParameter( size_t index );
		std::string GetArgumentDescription( size_t index );

		bool ResolveDepends( std::vector<PluginInfo*>& vec_plugin_info );
		bool Provides( const std::string& plugin_name, const std::string& plugin_version );
		int32_t VersionCompare( const std::string& version_1, const std::string& version_2 );
		void AddProvideForPlugin( PluginInfo* dependent ); // 添加到 vec_provides_info
		void RemoveProvideForPlugin( PluginInfo* dependent ); // 目前没有地方使用
		void DisableIndirectlyIfDependDisabled();

		std::vector<PluginInfo*> GetDependInfos() const; // 依赖项信息，在 resolved 以后生效
		std::vector<PluginInfo*> GetProvideInfos() const; // 依赖于本插件的插件列表

		bool LoadPluginLibrary();
		bool LoadPlugin( std::string library_name );
		bool UnloadPlugin();
		void SetPluginsX( Plugins_X* plugins_x );
		Plugins_X* GetPluginsX() const;
		bool PluginInitialize();
		bool PluginInitializeExt();
		void Stop();
		void Kill();

	public:
		bool m_enabled; // 是否加载 // 可以通过系统全局参数或插件信息文件的 Experimental 标签指定
		bool m_disabled_indirectly; // 是否使用
		bool m_experimental;
		bool m_has_error;
		std::vector<std::string> m_vec_error_info_list;

		std::string m_name;
		std::string m_version;
		std::string m_compat_version; // 适用版本号
		std::string m_vendor; // 开发者
		std::string m_copyright;
		std::string m_license;
		std::string m_category; // 相同分类则相同界面位置
		std::string m_description;
		std::string m_url;
		std::string m_location; // 插件所在文件夹
		std::string m_info_file_path; // 插件信息文件绝对路径
		std::string m_cfg_file_path; // 插件配置文件绝对路径

		std::vector<PluginDepend> m_vec_depend;
		std::vector<PluginArgument> m_vec_argument;

		std::vector<PluginInfo*> m_vec_provides_info;
		std::vector<PluginInfo*> m_vec_depends_info;

#ifdef __OS_WINDOWS__
		HINSTANCE m_instance;
#endif

		Plugins_X* m_plugins_x;

	private:
		SysLog_S* m_syslog;
		std::string m_log_cate;
		PluginInfo::State m_state;
	};

} // namespace basicx

#endif // BASICX_PLUGINS_PLUGIN_INFO_H
