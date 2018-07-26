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
#include <algorithm>

#include <common/assist.h>
#include <common/Format/Format.hpp>
#include <common/PugiXml/pugixml.hpp>

#include "plugin_info.h"

namespace basicx {

	PluginInfo::PluginInfo()
		: m_enabled( true )
		, m_disabled_indirectly( false )
		, m_experimental( false )
		, m_has_error( false )
		, m_instance( nullptr )
		, m_plugins_x( nullptr )
		, m_log_cate( "<PLUGIN_INFO>" )
		, m_state( PluginInfo::State::invalid ) {
		m_syslog = SysLog_S::GetInstance();
		m_vec_error_info_list.clear();
	}

	PluginInfo::~PluginInfo() {
	}

	void PluginInfo::LogPrint( syslog_level log_level, std::string& log_cate, std::string& log_info, int32_t log_show/* = 0*/ ) {
		m_syslog->LogWrite( log_level, log_cate, log_info );
		m_syslog->LogPrint( log_level, log_cate, "LOG>: " + log_info ); // ����̨
	}

	PluginInfo::State PluginInfo::GetState() const {
		return m_state;
	}

	void PluginInfo::SetEnabled( bool value ) {
		m_enabled = value;
	}

	void PluginInfo::SetDisabledIndirectly( bool value ) {
		m_disabled_indirectly = value;
	}

	void PluginInfo::SetExperimental( bool value ) {
		m_experimental = value;
	}

	void PluginInfo::SetError( bool value ) {
		m_has_error = value;
	}

	bool PluginInfo::IsEnabled() const {
		return m_enabled;
	}

	bool PluginInfo::IsDisabledIndirectly() const {
		return m_disabled_indirectly;
	}

	bool PluginInfo::IsExperimental() const {
		return m_experimental;
	}

	bool PluginInfo::HasError() const {
		return m_has_error;
	}

	void PluginInfo::AddErrorInfo( const std::string& error_info ) {
		m_has_error = true;
		m_vec_error_info_list.push_back( error_info );
	}

	std::vector<std::string> PluginInfo::GetErrorInfos() const {
		return m_vec_error_info_list;
	}

	bool PluginInfo::ReadPluginInfo( const std::string& info_file_path ) {
		m_name = "";
		m_version = "";
		m_compat_version = "";
		m_vendor = "";
		m_copyright = "";
		m_license = "";
		m_category = "";
		m_description = "";
		m_url = "";
		m_location = "";
		m_info_file_path = "";
		m_cfg_file_path = "";

		m_state = PluginInfo::State::invalid;
		//m_enabled = true;
		//m_disabled_indirectly = false;
		//m_experimental = false;
		m_has_error = false;
		m_vec_error_info_list.clear();

		if( ReadPluginInfoFromXML( info_file_path ) ) {
			m_state = PluginInfo::State::readed;
			return true;
		}

		return false;
	}

	bool PluginInfo::ReadPluginInfoFromXML( std::string info_file_path ) {
		std::string log_info;

		pugi::xml_document document;
		if( !document.load_file( info_file_path.c_str() ) ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "�򿪲����Ϣ�ļ�ʧ�ܣ�·����{0}", info_file_path );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		FormatLibrary::StandardLibrary::FormatTo( log_info, "�ɹ��򿪲����Ϣ�ļ���·����{0}", info_file_path );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		pugi::xml_node plugin_node = document.document_element();
		if( plugin_node.empty() ) {
			log_info = "��ȡ �����Ϣ ���ڵ� ʧ�ܣ�";
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}
		m_name = plugin_node.attribute( "Name" ).value();
		m_version = plugin_node.attribute( "Version" ).value();
		IsValidVersion( m_version, m_name );
		m_compat_version = plugin_node.attribute( "CompatVersion" ).value();
		IsValidVersion( m_compat_version, m_name );
		std::string experimental = plugin_node.attribute( "Experimental" ).value();
		std::transform( experimental.begin(), experimental.end(), experimental.begin(), tolower );
		if( "true" == experimental ) {
			m_enabled = false;
			m_experimental = true;
		}
		else if( "false" == experimental ) {
			m_enabled = true;
			m_experimental = false;
		}
		FormatLibrary::StandardLibrary::FormatTo( log_info, "�����Ϣ�ļ���Name��{0}��Version��{1}��CompatVersion��{2}", m_name, m_version, m_compat_version );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		m_vendor = plugin_node.child_value( "Vendor" );
		m_copyright = plugin_node.child_value( "Copyright" );
		m_license = plugin_node.child_value( "License" );
		m_category = plugin_node.child_value( "Category" );
		m_description = plugin_node.child_value( "Description" );
		m_url = plugin_node.child_value( "Url" );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "�����Ϣ�ļ���Vendor��{0}", m_vendor );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "�����Ϣ�ļ���Copyright��{0}", m_copyright );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "�����Ϣ�ļ���License��{0}", m_license );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "�����Ϣ�ļ���Category��{0}", m_category );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "�����Ϣ�ļ���Description��{0}", m_description );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
		FormatLibrary::StandardLibrary::FormatTo( log_info, "�����Ϣ�ļ���Url��{0}", m_url );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		// ��ȡ��� ���� �б���Ϣ
		m_vec_depend.clear();
		pugi::xml_node dependency_list_node = plugin_node.child( "DependencyList" );
		for( pugi::xml_node dependency_node = dependency_list_node.first_child(); !dependency_node.empty(); dependency_node = dependency_node.next_sibling() ) {
			PluginDepend plugin_depend;
			plugin_depend.m_name = dependency_node.attribute( "Name" ).value();
			plugin_depend.m_version = dependency_node.attribute( "Version" ).value();

			if( plugin_depend.m_name != "" && IsValidVersion( plugin_depend.m_version, m_name ) ) {
				m_vec_depend.push_back( plugin_depend );
				FormatLibrary::StandardLibrary::FormatTo( log_info, "��� ���� �Name��{0}��Version��{1}", plugin_depend.m_name, plugin_depend.m_version );
				m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
			}
		}

		// ��ȡ��� ���� �б���Ϣ
		m_vec_argument.clear();
		pugi::xml_node argument_list_node = plugin_node.child( "ArgumentList" );
		for( pugi::xml_node argument_node = argument_list_node.first_child(); !argument_node.empty(); argument_node = argument_node.next_sibling() ) {
			PluginArgument plugin_argument;
			plugin_argument.m_name = argument_node.attribute( "Name" ).value();
			plugin_argument.m_parameter = argument_node.attribute( "Parameter" ).value();

			if( plugin_argument.m_name != "" && plugin_argument.m_parameter != "" ) {
				plugin_argument.m_description = argument_node.child_value();
				m_vec_argument.push_back( plugin_argument );
				FormatLibrary::StandardLibrary::FormatTo( log_info, "��� ���� �Name��{0}��Parameter��{1}��Description��{2}", plugin_argument.m_name, plugin_argument.m_parameter, plugin_argument.m_description );
				m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
			}
		}

		return true;
	}

	bool PluginInfo::IsValidVersion( const std::string& version, const std::string& plugin_name ) {
		bool there_is_error = false;

		if( "" == version ) {
			there_is_error = true;
		}

		size_t version_length = version.length();

		size_t dot_count = 0;
		for( size_t i = 0; i < version_length; i++ ) {
			if( '.' == version.at( i ) ) {
				dot_count++;
			}
		}
		if( dot_count != 2 ) { // Ŀǰ�޶����� 3 λ�汾��
			there_is_error = true;
		}

		size_t pos_1 = version.find( '.' );
		size_t pos_2 = version.find( '.', pos_1 + 1 );
		int32_t value[3] = { 0 };
		if( pos_1 > 0 && pos_2 > 0 && pos_2 > pos_1 && pos_2 + 1 < version_length ) {
			value[0] = atoi( version.substr( 0, pos_1 ).c_str() );
			value[1] = atoi( version.substr( pos_1 + 1, pos_2 - pos_1 - 1 ).c_str() );
			value[2] = atoi( version.substr( pos_2 + 1, version_length - 1 - pos_2 ).c_str() );
			for( size_t i = 0; i < 3; i++ ) {
				if( value[i] < 0 ) {
					there_is_error = true;
					break;
				}
			}
		}
		else {
			there_is_error = true;
		}

		if( true == there_is_error ) {
			std::string log_info;
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0} ��Ϣ�ļ��а汾�Ż���������汾�ŷǷ���", plugin_name );
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		return true;
	}

	std::string PluginInfo::Name() const {
		return m_name;
	}

	std::string PluginInfo::Version() const {
		return m_version;
	}

	std::string PluginInfo::CompatVersion() const {
		return m_compat_version;
	}

	std::string PluginInfo::Vendor() const {
		return m_vendor;
	}

	std::string PluginInfo::Copyright() const {
		return m_copyright;
	}

	std::string PluginInfo::License() const {
		return m_license;
	}

	std::string PluginInfo::Category() const {
		return m_category;
	}

	std::string PluginInfo::Description() const {
		return m_description;
	}

	std::string PluginInfo::Url() const {
		return m_url;
	}

	void PluginInfo::SetLocation( std::string path_folder ) {
		m_location = path_folder;
	}

	void PluginInfo::SetInfoFilePath( std::string path_info_file ) {
		m_info_file_path = path_info_file;
	}

	void PluginInfo::SetCfgFilePath( std::string path_cfg_file ) {
		m_cfg_file_path = path_cfg_file;
	}

	std::string PluginInfo::GetLocation() const {
		return m_location;
	}

	std::string PluginInfo::GetInfoFilePath() const {
		return m_info_file_path;
	}

	std::string PluginInfo::GetCfgFilePath() const {
		return m_cfg_file_path;
	}

	std::vector<PluginDepend> PluginInfo::GetDepends() const {
		return m_vec_depend;
	}

	size_t PluginInfo::GetDependsNumber() {
		return m_vec_depend.size();
	}

	std::string PluginInfo::GetDependName( size_t index ) {
		if( index >= 0 && index < m_vec_depend.size() ) {
			return m_vec_depend[index].m_name;
		}
		return "";
	}

	std::string PluginInfo::GetDependVersion( size_t index ) {
		if( index >= 0 && index < m_vec_depend.size() ) {
			return m_vec_depend[index].m_version;
		}
		return "";
	}

	std::vector<PluginArgument> PluginInfo::GetArguments() const {
		return m_vec_argument;
	}

	size_t PluginInfo::GetArgumentsNumber() {
		return m_vec_argument.size();
	}

	std::string PluginInfo::GetArgumentName( size_t index ) {
		if( index >= 0 && index < m_vec_argument.size() ) {
			return m_vec_argument[index].m_name;
		}
		return "";
	}

	std::string PluginInfo::GetArgumentParameter( size_t index ) {
		if( index >= 0 && index < m_vec_argument.size() ) {
			return m_vec_argument[index].m_parameter;
		}
		return "";
	}

	std::string PluginInfo::GetArgumentDescription( size_t index ) {
		if( index >= 0 && index < m_vec_argument.size() ) {
			return m_vec_argument[index].m_description;
		}
		return "";
	}

	bool PluginInfo::ResolveDepends( std::vector<PluginInfo*>& vec_plugin_info ) {
		std::string log_info;

		if( m_has_error ) {
			return false;
		}

		if( PluginInfo::State::resolved == m_state ) { // ���֮ǰ�Ѿ���������������������
			m_state = PluginInfo::State::readed;
		}

		if( m_state != PluginInfo::State::readed ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��Ϊ��� {0}({1}) ״̬ m_state != readed ���Բ����������ʧ�ܣ�", m_name, m_version );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			AddErrorInfo( log_info );
			return false;
		}

		for( size_t i = 0; i < m_vec_depend.size(); i++ ) { // �����Ҫ�����Ĳ����Ϣ
			FormatLibrary::StandardLibrary::FormatTo( log_info, "����� {0}({1}) Ҫ�����Ĳ����{2}({3})", m_name, m_version, m_vec_depend[i].m_name, m_vec_depend[i].m_version );
			m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

			PluginInfo* plugin_info_found = nullptr;
			for( size_t j = 0; j < vec_plugin_info.size(); j++ ) { // ������ȡ���Ĳ����Ϣ
				if( vec_plugin_info[j]->Provides( m_vec_depend[i].m_name, m_vec_depend[i].m_version ) ) { // ���в���д��ڸ���������Ұ汾�ŷ�Χ����Ҫ��
					plugin_info_found = vec_plugin_info[j]; // ���в���еĸ�������������ṩ��
					vec_plugin_info[j]->AddProvideForPlugin( this ); // ���Լ���ӵ��ṩ�� vec_plugin_info[j] �� m_vec_provides_info ��
					m_vec_depends_info.push_back( vec_plugin_info[j] ); // ��� �ҵ��ķ��ϵ��ṩ�ߣ����������Ĳ������Ϣ ���б� (�� m_vec_depend ��Ԫ��ֻ������������������ֺͰ汾��Ϣ)

					FormatLibrary::StandardLibrary::FormatTo( log_info, "�ҵ��ṩ�ߣ�{0}({1})", vec_plugin_info[j]->Name(), vec_plugin_info[j]->Version() );
					m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );
					break;
				}
			}

			if( !plugin_info_found ) { // �Ҳ�������Ҫ��������������û���ṩ�� //m_vec_depends_info ��������ȱʧ���������
				FormatLibrary::StandardLibrary::FormatTo( log_info, "�޷��ҵ��ṩ�ߣ�{0}({1})", m_vec_depend[i].m_name, m_vec_depend[i].m_version );
				m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
				AddErrorInfo( log_info ); // m_has_error = true; �򱾲���� LoadPlugin(...) ʱ����������
			}
		}
		if( m_has_error ) { // �������������ȱʧ
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) �����������ȱʧ��\r\n", m_name, m_version );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) �������������ɡ�\r\n", m_name, m_version );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		m_state = PluginInfo::State::resolved;

		return true;
	}

	bool PluginInfo::Provides( const std::string& plugin_name, const std::string& plugin_version ) {
		if( plugin_name != m_name ) {
			return false;
		}

		int32_t result_1 = VersionCompare( m_version, plugin_version ); // Ҫ�� m_version >= plugin_version ������ 1 �� 0
		int32_t result_2 = VersionCompare( m_compat_version, plugin_version ); // Ҫ�� m_compat_version <= plugin_version ������ -1 �� 0

		std::string log_info;
		FormatLibrary::StandardLibrary::FormatTo( log_info, "Provides �� VersionCompare �����{0} {1}", result_1, result_2 );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		return ( 1 == result_1 || 0 == result_1 ) && ( -1 == result_2 || 0 == result_2 );
	}

	int32_t PluginInfo::VersionCompare( const std::string& version_1, const std::string& version_2 ) { // 1 > 2 ���� 1��1 < 2 ���� -1��1 == 2 ���� 0
		size_t version_length_1 = version_1.length();
		size_t version_length_2 = version_2.length();

		size_t pos_11 = version_1.find( '.' );
		size_t pos_12 = version_1.find( '.', pos_11 + 1 );
		int32_t value_1[3] = { 0 };
		// if( pos_11 > 0 && pos_12 > 0 && pos_12 > pos_11 && pos_12 + 1 < version_length_1 ) ���ڶ�ȡ�汾��ʱ�������
		value_1[0] = atoi( version_1.substr( 0, pos_11 ).c_str() );
		value_1[1] = atoi( version_1.substr( pos_11 + 1, pos_12 - pos_11 - 1 ).c_str() );
		value_1[2] = atoi( version_1.substr( pos_12 + 1, version_length_1 - 1 - pos_12 ).c_str() );

		size_t pos_21 = version_2.find( '.' );
		size_t pos_22 = version_2.find( '.', pos_21 + 1 );
		int32_t value_2[3] = { 0 };
		// if( pos_21 > 0 && pos_22 > 0 && pos_22 > pos_21 && pos_22 + 1 < version_length_2 ) ���ڶ�ȡ�汾��ʱ�������
		value_2[0] = atoi( version_2.substr( 0, pos_21 ).c_str() );
		value_2[1] = atoi( version_2.substr( pos_21 + 1, pos_22 - pos_21 - 1 ).c_str() );
		value_2[2] = atoi( version_2.substr( pos_22 + 1, version_length_2 - 1 - pos_22 ).c_str() );

		//std::string log_info;
		//FormatLibrary::StandardLibrary::FormatTo( log_info, "<xrdtest> VersionCompare��version_length_1��{0}��pos_11��{1}��pos_12��{2}��value_1[0]��{3}��value_1[1]��{4}��value_1[2]��{5}", version_length_1, pos_11, pos_12, value_1[0], value_1[1], value_1[2] );
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );
		//FormatLibrary::StandardLibrary::FormatTo( log_info, "<xrdtest> VersionCompare��version_length_2��{0}��pos_21��{1}��pos_22��{2}��value_2[0]��{3}��value_2[1]��{4}��value_2[2]��{5}", version_length_2, pos_21, pos_22, value_2[0], value_2[1], value_2[2] );
		//m_syslog->LogWrite( syslog_level::c_debug, m_log_cate, log_info );

		for( size_t i = 0; i < 3; i++ ) {
			if( value_1[i] > value_2[i] ) {
				return 1;
			}
			if( value_1[i] < value_2[i] ) {
				return -1;
			}
		}

		return 0; // ��λ�汾�Ŷ����
	}

	void PluginInfo::AddProvideForPlugin( PluginInfo* dependent ) { // dependent ����������
		m_vec_provides_info.push_back( dependent ); // �� this Ϊ�ṩ��
	}

	void PluginInfo::RemoveProvideForPlugin( PluginInfo* dependent ) { // Ŀǰû�еط�ʹ��
		int32_t position = -1;
		for( size_t i = 0; i < m_vec_provides_info.size(); i++ ) {
			if( m_vec_provides_info[i]->Name() == dependent->Name() ) {
				position = (int32_t)i;
				break;
			}
		}
		if( position >= 0 ) {
			for( int32_t i = position; i < m_vec_provides_info.size() - 1; i++ ) {
				m_vec_provides_info[i] = m_vec_provides_info[i + 1];
			}
		}
		m_vec_provides_info.pop_back();
	}

	void PluginInfo::DisableIndirectlyIfDependDisabled() {
		m_disabled_indirectly = false;

		if( !m_enabled ) {
			return;
		}

		for( size_t i = 0; i < m_vec_depends_info.size(); i++ ) {
			// m_enabled �� m_disabled_indirectly ��ֵ��֮ǰ��ϵͳ�����ļ�������Ϣ�ļ��� Experimental ���Զ�ȡ�������ʼ��ֵΪ m_enabled == true �� m_disabled_indirectly == false ��Ĭ��Ϊ����
			if( m_vec_depends_info[i]->IsDisabledIndirectly() || !m_vec_depends_info[i]->IsEnabled() ) { // �����޷����ص��������
				m_disabled_indirectly = true; // �򱾲���� LoadPlugin(...) ʱ��������
				std::string log_info;
				FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) �����޷����ص����������{2}({3})���ʽ��ñ������", m_name, m_version, m_vec_depends_info[i]->Name(), m_vec_depends_info[i]->Version() );
				m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
				break;
			}
		}
	}

	std::vector<PluginInfo*> PluginInfo::GetDependInfos() const {
		return m_vec_depends_info;
	}

	std::vector<PluginInfo*> PluginInfo::GetProvideInfos() const {
		return m_vec_provides_info;
	}

	bool PluginInfo::LoadPluginLibrary() {
		std::string log_info;

		if( m_has_error ) {
			return false;
		}

		if( m_state != PluginInfo::State::resolved ) {
			if( PluginInfo::State::loaded == m_state ) {
				return true;
			}

			log_info = "���ز�����ʧ�ܣ�m_state != resolved && m_state != loaded";
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		std::string library_name;
#ifdef _DEBUG
		FormatLibrary::StandardLibrary::FormatTo( library_name, "{0}\\{1}.dll", m_location, m_name );
#else
		FormatLibrary::StandardLibrary::FormatTo( library_name, "{0}\\{1}.dll", m_location, m_name );
#endif

		if( !LoadPlugin( library_name ) ) {
			return false;
		}

		// ����ָ���ɲ����ʼ��ʱ�Լ���ӣ����� Plugins::AddPluginSelf(...)
		//m_plugins_x = dynamic_cast<Plugins_X*>( m_instance );
		//m_plugins_x->SetPluginInfo( this );

		m_state = PluginInfo::State::loaded;

		return true;
	}

	bool PluginInfo::LoadPlugin( std::string library_name ) {
		std::string log_info;

		if( "" == library_name ) {
			log_info = "Ҫ���صĲ�����·��Ϊ�գ�";
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		if( m_instance != nullptr ) {
			return true;
		}

#ifdef __OS_WINDOWS__
		m_instance = LoadLibrary( StringToWideChar( library_name ).c_str() );
#endif
		
		if( nullptr == m_instance ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) ���������ʧ�ܣ�ʵ�����Ϊ�գ�·����{2}", m_name, m_version, library_name );
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) ��⵼��ɹ���·����{2}", m_name, m_version, library_name );
		m_syslog->LogWrite( syslog_level::c_info, m_log_cate, log_info );

		return true;
	}

	bool PluginInfo::UnloadPlugin() {
		if( nullptr == m_instance ) {
			return true;
		}

#ifdef __OS_WINDOWS__
		FreeLibrary( m_instance );
#endif

		return true;
	}

	void PluginInfo::SetPluginsX( Plugins_X* plugins_x ) {
		m_plugins_x = plugins_x;
	}

	Plugins_X* PluginInfo::GetPluginsX() const {
		return m_plugins_x;
	}

	bool PluginInfo::PluginInitialize() {
		std::string log_info;

		if( m_has_error ) {
			return false;
		}

		if( m_state != PluginInfo::State::loaded ) {
			if( PluginInfo::State::initialized == m_state ) {
				return true;
			}

			log_info = "��ʼ�����ʧ�ܣ�m_state != loaded && m_state != initialized";
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		if( !m_plugins_x ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) û��ʵ���ɹ���ʼ����", m_name, m_version );
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		if( !m_plugins_x->Initialize() ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) ��ʼ��ʧ�ܣ�", m_name, m_version );
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		m_state = PluginInfo::State::initialized;

		return true;
	}

	bool PluginInfo::PluginInitializeExt() {
		std::string log_info;

		if( m_has_error ) {
			return false;
		}

		if( m_state != PluginInfo::State::initialized ) {
			if( PluginInfo::State::running == m_state ) {
				return true;
			}

			log_info = "��չ ��ʼ�����ʧ�ܣ�m_state != initialized && m_state != running";
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		if( !m_plugins_x ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) û��ʵ���ɹ� ��չ ��ʼ����", m_name, m_version );
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		if( !m_plugins_x->InitializeExt() ) {
			FormatLibrary::StandardLibrary::FormatTo( log_info, "��� {0}({1}) ��չ ��ʼ��ʧ�ܣ�", m_name, m_version );
			AddErrorInfo( log_info );
			m_syslog->LogWrite( syslog_level::c_error, m_log_cate, log_info );
			return false;
		}

		m_state = PluginInfo::State::running;

		return true;
	}

	void PluginInfo::Stop() {
		if( !m_plugins_x ) {
			return;
		}

		m_plugins_x->UninitializeExt(); // �����Ƿ��Ϊ����� StopPlugin() ������

		m_state = PluginInfo::State::stopped;
	}

	void PluginInfo::Kill() {
		if( !m_plugins_x ) {
			return;
		}

		m_plugins_x->Uninitialize();

		UnloadPlugin();

		// delete m_plugins_x; // FreeLibrary() �������ɾ��
		m_plugins_x = nullptr;

		m_state = PluginInfo::State::deleted;
	}

} // namespace basicx
