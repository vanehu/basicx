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

#include <iostream>

#include <common/sysdef.h>
#include <common/Format/Format.hpp>

#include "plugins_.h"

namespace basicx {

	Plugins_X::Plugins_X() {
	}

	Plugins_X::Plugins_X( std::string plugin_name ) {
		Plugins::GetInstance()->SetPluginsX( plugin_name, this );
	}

	Plugins_X::~Plugins_X() {
	}

	Plugins_P::Plugins_P()
		: m_info_file_ext( "xml" )
		, m_plugin_folder( "" )
		, m_plugins_running( false )
		, m_log_cate( "<PLUGINS>" ) {
		m_syslog = SysLog_S::GetInstance();
		//m_mission = Mission::GetInstance(); //mission//
	}

	Plugins_P::~Plugins_P() {
		StopAll();
		for( size_t i = 0; i < m_vec_plugin_info.size(); i++ ) {
			delete m_vec_plugin_info[i];
			m_vec_plugin_info[i] = nullptr;
		}
		m_vec_plugin_info.clear();
	}

	void Plugins_P::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show/* = 0*/ ) {
		m_syslog->LogWrite( log_level, log_cate, log_info );
		m_syslog->LogPrint( log_level, log_cate, "LOG>: " + log_info ); // 控制台
	}

	void Plugins_P::StartPlugins() {
		m_plugins_running = true;
	}

	bool Plugins_P::IsPluginsStarted() {
		return m_plugins_running;
	}

	std::vector<std::string> Plugins_P::GetErrorInfos() const {
		return m_vec_error_info_list;
	}

	bool Plugins_P::AddErrorInfo( const std::string& error_info ) {
		m_vec_error_info_list.push_back( error_info );
		return true;
	}

	void Plugins_P::SetInfoFileExt( const std::string& info_file_ext ) {
		m_info_file_ext = info_file_ext;
	}

	std::string Plugins_P::GetInfoFileExt() const {
		return m_info_file_ext;
	}

	void Plugins_P::SetPluginFolder( const std::string& plugin_folder ) {
		m_plugin_folder = plugin_folder;
	}

	std::string Plugins_P::GetPluginFolder() const {
		return m_plugin_folder;
	}

	void Plugins_P::SetPluginInfoPaths( const std::string& plugin_folder ) {
		SetPluginFolder( plugin_folder );
		m_vec_plugin_info_path.clear();
		FindPluginPaths( plugin_folder );

		std::string log_info;
		FormatLibrary::StandardLibrary::FormatTo( log_info, "共找到插件信息文件 {0} 个。\r\n", m_vec_plugin_info_path.size() );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
	}

	void Plugins_P::FindPluginPaths( std::string folder_path ) {
#ifdef __OS_WINDOWS__
		int32_t number = 0;
		WIN32_FIND_DATA find_data;
		ZeroMemory( &find_data, sizeof( WIN32_FIND_DATA ) );
		number = MultiByteToWideChar( 0, 0, folder_path.c_str(), -1, NULL, 0 );
		wchar_t* temp_folder_path = new wchar_t[number];
		MultiByteToWideChar( 0, 0, folder_path.c_str(), -1, temp_folder_path, number );
		std::wstring search_file = std::wstring( temp_folder_path ) + L"\\*.*";
		delete[] temp_folder_path;
		void* find_file = FindFirstFile( search_file.c_str(), &find_data );
		if( find_file != INVALID_HANDLE_VALUE ) {
			bool is_finish = FindNextFile( find_file, &find_data ) ? false : true;
			while( !is_finish ) {
				number = WideCharToMultiByte( CP_OEMCP, NULL, find_data.cFileName, -1, NULL, 0, NULL, FALSE );
				char* temp_file_name = new char[number];
				WideCharToMultiByte( CP_OEMCP, NULL, find_data.cFileName, -1, temp_file_name, number, NULL, FALSE );
				std::string file_name = std::string( temp_file_name );
				delete[] temp_file_name;
				if( file_name != "." && file_name != ".." ) {
					if( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) { // 子目录
						FindPluginPaths( folder_path + "\\" + file_name );
					}
					else { // 文件
						if( file_name.substr( file_name.rfind( '.' ) + 1 ) == m_info_file_ext ) { // 文件后缀符合
							m_vec_plugin_info_path.push_back( folder_path + "\\" + file_name );

							std::string log_info;
							FormatLibrary::StandardLibrary::FormatTo( log_info, "找到插件信息文件，路径：{0}", folder_path + "\\" + file_name );
							m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
						}
					}
				}
				is_finish = FindNextFile( find_file, &find_data ) ? false : true;
			}
		}
		FindClose( find_file );
#endif
	}

	std::vector<std::string> Plugins_P::GetPluginInfoPaths() const {
		return m_vec_plugin_info_path;
	}

	bool Plugins_P::LoadPluginSetting() {
		m_vec_disabled_plugin.clear();
		m_vec_force_enabled_plugin.clear();
		// TODO：从全局配置信息结构体获取禁用和强制使用的插件列表
		return true;
	}

	std::vector<std::string> Plugins_P::GetDisabledPlugins() const {
		return m_vec_disabled_plugin;
	}

	std::vector<std::string> Plugins_P::GetForceEnabledPlugins() const {
		return m_vec_force_enabled_plugin;
	}

	bool Plugins_P::WritePluginSetting() {
		// TODO：将禁用和强制使用的插件列表写入全局配置信息结构体和文件
		return true;
	}

	void Plugins_P::LoadPluginInfos() {
		std::string log_info;

		for( size_t i = 0; i < m_vec_plugin_info.size(); i++ ) {
			delete m_vec_plugin_info[i];
			m_vec_plugin_info[i] = nullptr;
		}
		m_vec_plugin_info.clear();

		for( size_t i = 0; i < m_vec_plugin_info_path.size(); i++ ) {
			PluginInfo* plugin_info = new PluginInfo();
			plugin_info->ReadPluginInfo( m_vec_plugin_info_path[i] ); //

			size_t pos_index = 0;
			pos_index = m_vec_plugin_info_path[i].rfind( '\\' );
			plugin_info->SetLocation( m_vec_plugin_info_path[i].substr( 0, pos_index ) ); // 插件所在文件夹路径 // 无"\"
			plugin_info->SetInfoFilePath( m_vec_plugin_info_path[i] ); // 插件信息文件路径

			FormatLibrary::StandardLibrary::FormatTo( log_info, "设置插件文件夹 路径：{0}", m_vec_plugin_info_path[i].substr( 0, pos_index ) );
			m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
			FormatLibrary::StandardLibrary::FormatTo( log_info, "设置插件信息文件 路径：{0}", m_vec_plugin_info_path[i] );
			m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

			pos_index = m_vec_plugin_info_path[i].rfind( '.' );
			plugin_info->SetCfgFilePath( m_vec_plugin_info_path[i].substr( 0, pos_index ) + ".ini" ); // 插件配置文件路径

			FormatLibrary::StandardLibrary::FormatTo( log_info, "设置插件配置文件 路径：{0}\r\n", m_vec_plugin_info_path[i].substr( 0, pos_index ) + ".ini" );
			m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

			// 以下说明系统配置文件优先级高于插件信息文件
			if( plugin_info->IsExperimental() ) { // 虽然是实验版本，但在系统配置文件的强制使用列表中
				for( size_t j = 0; j < m_vec_force_enabled_plugin.size(); j++ ) {
					if( m_vec_force_enabled_plugin[j] == plugin_info->Name() ) {
						plugin_info->SetEnabled( true );
						FormatLibrary::StandardLibrary::FormatTo( log_info, "根据系统配置信息，强制 使用 插件：{0}", plugin_info->Name() );
						m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
						break;
					}
				}
			}
			if( !plugin_info->IsExperimental() ) { // 虽然是正式版本，但在系统配置文件的禁止使用列表中
				for( size_t j = 0; j < m_vec_disabled_plugin.size(); j++ ) {
					if( m_vec_disabled_plugin[j] == plugin_info->Name() ) {
						plugin_info->SetEnabled( false );
						FormatLibrary::StandardLibrary::FormatTo( log_info, "根据系统配置信息，强制 禁用 插件：{0}", plugin_info->Name() );
						m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
						break;
					}
				}
			}

			if( !plugin_info->HasError() ) {
				m_vec_plugin_info.push_back( plugin_info ); // 添加到插件信息列表
			}
		}

		ResolveDepends();
	}

	void Plugins_P::ResolveDepends() {
		std::string log_info;

		//log_info = "<xrdtest> 进行 ResolveDepends()";
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );
		for( size_t i = 0; i < m_vec_plugin_info.size(); i++ ) {
			m_vec_plugin_info[i]->ResolveDepends( m_vec_plugin_info );
		}
		//log_info = "<xrdtest> ResolveDepends() 第一步完成";
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );

		// m_vec_plugin_info 中各个插件信息的 m_vec_depends_info 不包含上一步 ResolveDepends( m_vec_plugin_info ) 时可能缺失的依赖插件
		std::vector<PluginInfo*> vec_plugin_info_temp = LoadQueue();
		//log_info = "<xrdtest> ResolveDepends() LoadQueue() 完成";
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );
		for( size_t i = 0; i < vec_plugin_info_temp.size(); i++ ) {
			vec_plugin_info_temp[i]->DisableIndirectlyIfDependDisabled();
		}
		//log_info = "<xrdtest> ResolveDepends() 第二步完成";
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );

		log_info = "循环依赖检测完成。\r\n";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
	}

	std::vector<PluginInfo*> Plugins_P::LoadQueue() { // 返回通过循环依赖检测的插件列表
		std::vector<PluginInfo*> queue;
		for( size_t i = 0; i < m_vec_plugin_info.size(); i++ ) {
			std::string log_info;
			FormatLibrary::StandardLibrary::FormatTo( log_info, "循环依赖检测，插件：{0}({1})", m_vec_plugin_info[i]->Name(), m_vec_plugin_info[i]->Version() );
			m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

			std::vector<PluginInfo*> cycle_check_queue;
			LoadQueue( m_vec_plugin_info[i], queue, cycle_check_queue );
		}
		return queue;
	}

	bool Plugins_P::LoadQueue( PluginInfo* plugin_info, std::vector<PluginInfo*>& queue, std::vector<PluginInfo*>& cycle_check_queue ) {
		std::string log_info;

		for( size_t i = 0; i < queue.size(); i++ ) {
			if( queue[i] == plugin_info ) { // 依赖路径上的插件已通过循环依赖检查(在 queue 中，此插件的依赖也都OK，终止此依赖路径递归)
				return true;
			}
		}

		// 检查循环依赖
		// cycle_check_queue 列表包含：queue(已通过循环依赖检查的插件) 和 需要进行检查的依赖路径上的插件(这些插件都还没有在 queue 中)
		// 关键是用这些依赖路径上还未通过检查的去比对后继的依赖项，后继依赖项未出现过则加入列表继续往后递归，如果在 queue 中则此路径结束查看其他兄弟依赖项路径，如果只在 cycle_check_queue 中而 queue 中没有则发生了循环依赖
		for( size_t i = 0; i < cycle_check_queue.size(); i++ ) {
			if( cycle_check_queue[i] == plugin_info ) { // 如果满足，表示这个插件依赖于依赖路径上未通过循环依赖检查的插件(不在 queue 中却在 cycle_check_queue 中)
				std::string error_temp;
				log_info = "检测到循环依赖：";
				for( ; i < cycle_check_queue.size(); i++ ) {
					FormatLibrary::StandardLibrary::FormatTo( error_temp, "{0}({1}) 依赖于 ", cycle_check_queue[i]->Name(), cycle_check_queue[i]->Version() );
					log_info += error_temp;
				}
				FormatLibrary::StandardLibrary::FormatTo( error_temp, "{0}({1})！", plugin_info->Name(), plugin_info->Version() ); // 此时 plugin_info 尚未添加至 cycle_check_queue
				log_info += error_temp;
				plugin_info->AddErrorInfo( log_info );
				m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
				return false;
			}
		}
		cycle_check_queue.push_back( plugin_info ); // 可以说是顺序添加的
		//FormatLibrary::StandardLibrary::FormatTo( log_info, "<xrdtest> 添加 {0} 至 cycle_check_queue", plugin_info->Name() );
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );

		// 如果还是这两个状态，则没有进行过 ResolveDepends( m_vec_plugin_info )，则依赖插件队列为空，故不用再检查了
		if( PluginInfo::State::invalid == plugin_info->GetState() || PluginInfo::State::readed == plugin_info->GetState() ) {
			queue.push_back( plugin_info );
			//FormatLibrary::StandardLibrary::FormatTo( log_info, "<xrdtest> 添加 {0} 至 cycle_check_queue 状态为 invalid 或 readed", plugin_info->Name() );
			//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );
			return false;
		}

		// 递归检查依赖插件的循环依赖
		std::vector<PluginInfo*> vec_dependency_info_temp = plugin_info->GetDependInfos();
		for( size_t i = 0; i < vec_dependency_info_temp.size(); i++ ) {
			if( !LoadQueue( vec_dependency_info_temp[i], queue, cycle_check_queue ) ) {
				FormatLibrary::StandardLibrary::FormatTo( log_info, "存在循环依赖，无法加载插件：{0}({1})！", vec_dependency_info_temp[i]->Name(), vec_dependency_info_temp[i]->Version() );
				vec_dependency_info_temp[i]->AddErrorInfo( log_info );
				m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
				return false;
			}
		}

		// 将通过检查的插件信息加入到 queue
		queue.push_back( plugin_info ); // 可以说是逆序添加的，其 vecDependencyInfo 中后继的依赖项都已OK
		FormatLibrary::StandardLibrary::FormatTo( log_info, "<xrdtest> 添加 {0} 至 queue", plugin_info->Name() );
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );

		return true;
	}

	std::vector<PluginInfo*> Plugins_P::GetPluginInfos() const {
		return m_vec_plugin_info;
	}

	size_t Plugins_P::GetPluginInfosNumber() {
		return m_vec_plugin_info.size();
	}

	PluginInfo* Plugins_P::GetPluginInfo( size_t index ) {
		if( index >= 0 && index < m_vec_plugin_info.size() ) {
			return m_vec_plugin_info[index];
		}
		return nullptr;
	}

	PluginInfo* Plugins_P::FindPluginInfoByName( const std::string& plugin_name ) {
		for( size_t i = 0; i < m_vec_plugin_info.size(); i++ ) {
			if( m_vec_plugin_info[i]->Name() == plugin_name ) {
				return m_vec_plugin_info[i];
			}
		}
		return nullptr;
	}

	bool Plugins_P::LoadAll( std::string folder ) {
		std::string log_info;
		
		log_info = "开始加载所有插件 ...";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );
		
		SetInfoFileExt( "xml" );
		SetPluginInfoPaths( folder );
		LoadPluginSetting();
		LoadPluginInfos();

		// 查验核心插件
		//if( !FindPluginInfoByName( "PluginCore" ) ) {
		//	log_info = "无法找到系统核心插件信息！";
		//	LogPrint( syslog_level::c_error, m_log_cate, log_info );
		//	return false;
		//}
		//if( FindPluginInfoByName( "PluginCore" )->HasError() ) {
		//	log_info = "系统核心插件信息存在错误！";
		//	LogPrint( syslog_level::c_error, m_log_cate, log_info );
		//	return false;
		//}

		LoadPlugins(); // 加载所有插件

		// 查验加载结果
		//if( FindPluginInfoByName( "PluginCore" )->HasError() ) {
		//	log_info = "系统核心插件 加载 发生错误！";
		//	LogPrint( syslog_level::c_error, m_log_cate, log_info );
		//	return false;
		//}
		std::vector<PluginInfo*> vec_plugin_info = GetPluginInfos();
		for( size_t i = 0; i < vec_plugin_info.size(); i++ ) {
			if( vec_plugin_info[i]->HasError() && vec_plugin_info[i]->IsEnabled() && !vec_plugin_info[i]->IsDisabledIndirectly() ) { // 这里只检查可以加载的插件
				FormatLibrary::StandardLibrary::FormatTo( log_info, "插件 {0}({1}) 加载 发生错误！", vec_plugin_info[i]->Name(), vec_plugin_info[i]->Version() );
				LogPrint( syslog_level::c_error, m_log_cate, log_info );
			}
			else {
				PluginItem plugin_item;
				plugin_item.m_name = vec_plugin_info[i]->Name();
				plugin_item.m_state = "Loaded";
				plugin_item.m_version = vec_plugin_info[i]->Version();
				plugin_item.m_category = vec_plugin_info[i]->Category();
				plugin_item.m_description = vec_plugin_info[i]->Description();
				plugin_item.m_dependence = "No Dependence";
				size_t plugin_depend_number = vec_plugin_info[i]->GetDependsNumber();
				if( plugin_depend_number > 0 ) {
					plugin_item.m_dependence = "";
					for( size_t j = 0; j < plugin_depend_number; j++ ) {
						std::string value_temp;
						FormatLibrary::StandardLibrary::FormatTo( value_temp, "{0}({1}) ", vec_plugin_info[i]->GetDependName( j ), vec_plugin_info[i]->GetDependVersion( j ) );
						plugin_item.m_dependence += value_temp;
					}
				}
			}
		}

		log_info = "所有插件加载完成。";
		LogPrint( syslog_level::c_info, m_log_cate, log_info );

		return true;
	}

	void Plugins_P::LoadPlugins() {
		std::string log_info;
		
		std::vector<PluginInfo*> queue = LoadQueue();
		log_info = "插件 导入队列创建 完成。\r\n";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		
		for( size_t i = 0; i < queue.size(); i++ ) {
			LoadPlugin( queue[i], PluginInfo::State::loaded );
		}
		log_info = "插件 动态链接库导入 完成。\r\n";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		
		for( size_t i = 0; i < queue.size(); i++ ) {
			LoadPlugin( queue[i], PluginInfo::State::initialized );
		}
		log_info = "插件 初始化 完成。\r\n";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		
		for( int32_t i = (int32_t)queue.size() - 1; i >= 0; i-- ) { // 逆序 // int32_t：可能为负的
			LoadPlugin( queue[i], PluginInfo::State::running );
		}
		log_info = "插件 扩展初始化 完成。\r\n";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
	}

	void Plugins_P::LoadPlugin( PluginInfo* plugin_info, PluginInfo::State dest_state ) { // 这之前有错误发生的插件不被加载会不会导致什么问题？
		if( plugin_info->HasError() || (int32_t)plugin_info->GetState() != (int32_t)dest_state - 1 ) {
			return;
		}
		
		if( ( plugin_info->IsDisabledIndirectly() || !plugin_info->IsEnabled() ) && PluginInfo::State::loaded == dest_state ) {
			return;
		}
		
		switch( dest_state ) {
		case PluginInfo::State::running:
			plugin_info->PluginInitializeExt();
			return;
		case PluginInfo::State::deleted:
			plugin_info->Kill();
			return;
		default:
			break;
		}
		
		std::vector<PluginInfo*> vec_dependency_info_temp = plugin_info->GetDependInfos();
		for( size_t i = 0; i < vec_dependency_info_temp.size(); i++ ) {
			if( vec_dependency_info_temp[i]->GetState() != dest_state ) {
				std::string log_info;
				FormatLibrary::StandardLibrary::FormatTo( log_info, "因为依赖插件 {0}({1}) 没有加载或停止，所以无法加载或停止插件 {2}({3})！", vec_dependency_info_temp[i]->Name(), vec_dependency_info_temp[i]->Version(), plugin_info->Name(), plugin_info->Version() );
				plugin_info->AddErrorInfo( log_info );
				m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
				return;
			}
		}
		
		switch( dest_state ) {
		case PluginInfo::State::loaded:
			plugin_info->LoadPluginLibrary();
			break;
		case PluginInfo::State::initialized:
			plugin_info->PluginInitialize();
			break;
		case PluginInfo::State::stopped:
			plugin_info->Stop();
			break;
		default:
			break;
		}
	}

	void Plugins_P::StopAll() {
		std::string log_info;

		std::vector<PluginInfo*> queue = LoadQueue();
		log_info = "插件 导入队列创建 完成。";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		for( size_t i = 0; i < queue.size(); i++ ) {
			LoadPlugin( queue[i], PluginInfo::State::stopped );
		}
		log_info = "所有插件 停止运行。";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		for( int32_t i = (int32_t)queue.size() - 1; i >= 0; i-- ) { // 逆序 // int32_t：可能为负的
			LoadPlugin( queue[i], PluginInfo::State::deleted );
		}
		log_info = "所有插件 对象删除。\r\n\r\n";
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
	}

	Plugins_X* Plugins_P::GetPluginsX( const std::string& plugin_name ) {
		PluginInfo* plugin_info = FindPluginInfoByName( plugin_name );
		if( plugin_info != nullptr ) {
			return plugin_info->GetPluginsX();
		}
		return nullptr;
	}

	void Plugins_P::SetPluginsX( const std::string& plugin_name, Plugins_X* plugins_x ) {
		PluginInfo* plugin_info = FindPluginInfoByName( plugin_name );
		if( plugin_info != nullptr ) {
			plugin_info->SetPluginsX( plugins_x );
		}
	}

	std::string Plugins_P::GetPluginLocationByName( const std::string& plugin_name ) {
		PluginInfo* plugin_info = FindPluginInfoByName( plugin_name );
		if( plugin_info != nullptr ) {
			return plugin_info->GetLocation();
		}
		return "";
	}

	std::string Plugins_P::GetPluginCfgFilePathByName( const std::string& plugin_name ) {
		PluginInfo* plugin_info = FindPluginInfoByName( plugin_name );
		if( plugin_info != nullptr ) {
			return plugin_info->GetCfgFilePath();
		}
		return "";
	}

	std::string Plugins_P::GetPluginInfoFilePathByName( const std::string& plugin_name ) {
		PluginInfo* plugin_info = FindPluginInfoByName( plugin_name );
		if( plugin_info != nullptr ) {
			return plugin_info->GetInfoFilePath();
		}
		return "";
	}

	//void Plugins_P::AddUserTaskSend( int32_t task_type, std::string& task_info, std::string& task_data/* = string( "" )*/ ) {
	//	if( "" == task_data ) { // 无附带数据
	//		m_plugins_p->m_mission->RecvTaskMsg( task_type, task_info );
	//	}
	//	else { // 有附带数据
	//		m_plugins_p->m_mission->RecvTaskMsg( task_type, task_info, task_data );
	//	}
	//}

	void Plugins_P::OnDeliverTask( int32_t task_id, std::string node_type, int32_t identity, int32_t code, std::string& data ) {
		PluginInfo* plugin_info = FindPluginInfoByName( node_type );
		if( plugin_info != nullptr ) {
			Plugins_X* plugins_x = plugin_info->GetPluginsX();
			if( plugins_x != nullptr ) {
				plugins_x->AssignTask( task_id, identity, code, data );
			}
		}
	}

	void Plugins_P::CommitResult( int32_t task_id, int32_t identity, int32_t code, std::string& data ) {
		//m_plugins_p->m_mission->OnTaskResult( task_id, identity, code, data ); //mission//
	}

	Plugins* Plugins::m_instance = nullptr;

	Plugins::Plugins() //Plugins::Plugins() : Mission_X() //mission//
		: m_plugins_p( nullptr ) {
		try {
			m_plugins_p = new Plugins_P();
		}
		catch( ... ) {}
		m_instance = this;
	}

	Plugins::~Plugins() {
		if( m_plugins_p != nullptr ) {
			delete m_plugins_p;
			m_plugins_p = nullptr;
		}
	}

	Plugins* Plugins::GetInstance() {
		return m_instance;
	}

	void Plugins::StartPlugins() {
		m_plugins_p->StartPlugins();
	}

	bool Plugins::IsPluginsStarted() {
		return m_plugins_p->IsPluginsStarted();
	}

	bool Plugins::LoadAll( std::string folder ) {
		return m_plugins_p->LoadAll( folder );
	}

	void Plugins::StopAll() {
		m_plugins_p->StopAll();
	}

	Plugins_X* Plugins::GetPluginsX( const std::string& plugin_name ) const {
		return m_plugins_p->GetPluginsX( plugin_name );
	}

	void Plugins::SetPluginsX( const std::string& plugin_name, Plugins_X* plugins_x ) {
		m_plugins_p->SetPluginsX( plugin_name, plugins_x );
	}

	std::string Plugins::GetPluginLocationByName( const std::string& plugin_name ) const {
		return m_plugins_p->GetPluginLocationByName( plugin_name );
	}

	std::string Plugins::GetPluginCfgFilePathByName( const std::string& plugin_name ) const {
		return m_plugins_p->GetPluginCfgFilePathByName( plugin_name );
	}

	std::string Plugins::GetPluginInfoFilePathByName( const std::string& plugin_name ) const {
		return m_plugins_p->GetPluginInfoFilePathByName( plugin_name );
	}

} // namespace basicx
