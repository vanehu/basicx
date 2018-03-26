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

#ifndef BASICX_COMMON_TESTER_H
#define BASICX_COMMON_TESTER_H

#include <iostream>

#include "common.h"

namespace basicx {

	void Test_Common() {
		std::string str;
		std::string cfg_file_path = "..//etc//cfg_main.ini";
		pugi::xml_document xmlDocument;
		if( !xmlDocument.load_file( cfg_file_path.c_str() ) ) {
			FormatLibrary::StandardLibrary::FormatTo( str, "打开插件参数配置信息文件失败！{0}", cfg_file_path );
			std::cout << str << std::endl << std::endl;
			return;
		}
		pugi::xml_node xmlPluginNode = xmlDocument.document_element();
		if( xmlPluginNode.empty() ) {
			std::cout << "获取插件参数配置信息 根节点 失败！" << std::endl << std::endl;
			return;
		}
		std::string trade_front = xmlPluginNode.child_value( "TradeFront" );
		std::string broker_id = xmlPluginNode.child_value( "BrokerID" );
		int32_t query_only = atoi( xmlPluginNode.child_value( "QueryOnly" ) );
		FormatLibrary::StandardLibrary::FormatTo( str, "{0} {1} {2}", trade_front, broker_id, query_only );
		std::cout << str << std::endl << std::endl;
		for( pugi::xml_node xmlRiskerChildNode = xmlPluginNode.first_child(); !xmlRiskerChildNode.empty(); xmlRiskerChildNode = xmlRiskerChildNode.next_sibling() ) {
			if( "Risker" == std::string( xmlRiskerChildNode.name() ) ) {
				std::string account = xmlRiskerChildNode.attribute( "Account" ).value();
				std::string asset_account = xmlRiskerChildNode.attribute( "AssetAccount" ).value();
				FormatLibrary::StandardLibrary::FormatTo( str, "{0} {1}", account, asset_account );
				std::cout << str << std::endl << std::endl;
			}
		}
		std::cout << "插件参数配置信息读取完成。" << std::endl << std::endl;
	}

} // namespace basicx

#endif // BASICX_COMMON_TESTER_H
