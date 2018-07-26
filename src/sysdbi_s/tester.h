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

#ifndef BASICX_SYSDBI_S_TESTER_H
#define BASICX_SYSDBI_S_TESTER_H

#include <iostream>
#include <algorithm>

#include "sysdbi_s.h"

#import "C:\Program Files (x86)\Common Files\System\ado\msado20.tlb" named_guids rename("EOF", "adoEOF") // SQL Server

namespace basicx {

	void Test_SysDBI_S() {
		int32_t result = S_OK;
		ADODB::_Connection* connection = nullptr;
		ADODB::_Recordset* recordset = nullptr;
		basicx::SysDBI_S* sysdbi_s = basicx::SysDBI_S::GetInstance();
		result = sysdbi_s->Connect( connection, recordset, "10.0.7.80", 1433, "research", "Research@123", "JYDB_NEW" );
		if( FAILED( result ) ) {
			std::cout << "连接数据库失败！" << result << std::endl;
			return;
		}
		else {
			std::cout << "连接数据库成功。" << result << std::endl;
		}

		std::string sql_query = "select ContractCode, ExchangeCode from [JYDB_NEW].[dbo].[Fut_ContractMain] A \
					             where ( A.ExchangeCode = 10 or A.ExchangeCode = 13 or A.ExchangeCode = 15 or A.ExchangeCode = 20 ) \
					             and A.EffectiveDate <= CONVERT( VARCHAR( 10 ), GETDATE(), 120 ) \
					             and A.LastTradingDate >= CONVERT( VARCHAR( 10 ), GETDATE(), 120 ) order by A.LastTradingDate";
		if( true == sysdbi_s->Query( connection, recordset, sql_query ) ) {
			int64_t row_number = sysdbi_s->GetCount( recordset );
			if( row_number <= 0 ) {
				std::cout << "查询获得合约代码记录条数异常！" << row_number << std::endl;
			}
			else {
				while( !sysdbi_s->GetEOF( recordset ) ) {
					_variant_t v_contract_code;
					_variant_t v_exchange_code;
					v_contract_code = recordset->GetCollect( "ContractCode" );
					v_exchange_code = recordset->GetCollect( "ExchangeCode" );
					std::string contract_code = _bstr_t( v_contract_code );
					int32_t exchange_code = 0;
					if( v_exchange_code.vt != VT_NULL ) {
						exchange_code = (int32_t)v_exchange_code;
					} // 10-上海期货交易所，13-大连商品交易所，15-郑州商品交易所，20-中国金融期货交易所
					if( contract_code != "" ) {
						// 查询所得均为大写
						// 上期所和大商所品种代码需小写：10、13
						// 中金所和郑商所品种代码需大写：15、20
						if( 10 == exchange_code || 13 == exchange_code ) {
							std::transform( contract_code.begin(), contract_code.end(), contract_code.begin(), tolower );
						}
						std::cout << "合约代码：" << contract_code << " " << exchange_code << std::endl;
					}
					sysdbi_s->MoveNext( recordset );
				}

				std::cout << "查询获得合约代码记录：" << row_number << std::endl;
			}
		}
		else {
			std::cout << "查询合约代码记录失败！" << std::endl;
		}

		sysdbi_s->Close( recordset );
		sysdbi_s->Release( connection, recordset );
	}

} // namespace basicx

#endif // BASICX_SYSDBI_S_TESTER_H
