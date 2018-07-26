
# Copyright (c) 2017-2018 the BasicX authors
# All rights reserved.
#
# The project sponsor and lead author is Xu Rendong.
# E-mail: xrd@ustc.edu, QQ: 277195007, WeChat: ustc_xrd
# See the contributors file for names of other contributors.
#
# Commercial use of this code in source and binary forms is
# governed by a LGPL v3 license. You may get a copy from the
# root directory. Or else you should get a specific written 
# permission from the project author.
#
# Individual and educational use of this code in source and
# binary forms is governed by a 3-clause BSD license. You may
# get a copy from the root directory. Certainly welcome you
# to contribute code of all sorts.
#
# Be sure to retain the above copyright notice and conditions.

FILE (GLOB COMMON_HEADER_FILES "./*.h")
FILE (GLOB COMMON_SOURCE_FILES "./*.cpp")
FILE (GLOB COMMON_OTLV4_SOURCE_FILES "./OTLV4/*.h")
FILE (GLOB COMMON_FORMAT_SOURCE_FILES "./Format/*.hpp")
FILE (GLOB COMMON_PUGIXML_SOURCE_FILES "./PugiXml/*.hpp" "./PugiXml/*.cpp")
FILE (GLOB COMMON_JSONCPP_SOURCE_FILES "./JsonCpp/*.h" "./JsonCpp/*.inl" "./JsonCpp/*.cpp")
FILE (GLOB COMMON_HIREDIS_SOURCE_FILES "./Hiredis/*.h" "./Hiredis/*.c" "./Hiredis/Win32_Interop/*.h" "./Hiredis/Win32_Interop/*.c" "./Hiredis/Win32_Interop/*.cpp")

SOURCE_GROUP ("Header Files" FILES ${COMMON_HEADER_FILES})
SOURCE_GROUP ("Source Files" FILES ${COMMON_SOURCE_FILES})
SOURCE_GROUP ("OTLV4" FILES ${COMMON_OTLV4_SOURCE_FILES})
SOURCE_GROUP ("Format" FILES ${COMMON_FORMAT_SOURCE_FILES})
SOURCE_GROUP ("PugiXml" FILES ${COMMON_PUGIXML_SOURCE_FILES})
SOURCE_GROUP ("JsonCpp" FILES ${COMMON_JSONCPP_SOURCE_FILES})
SOURCE_GROUP ("Hiredis" FILES ${COMMON_HIREDIS_SOURCE_FILES})

ADD_LIBRARY ("common" ${COMMON_HEADER_FILES} ${COMMON_SOURCE_FILES} ${COMMON_OTLV4_SOURCE_FILES} ${COMMON_FORMAT_SOURCE_FILES} ${COMMON_PUGIXML_SOURCE_FILES} ${COMMON_JSONCPP_SOURCE_FILES} ${COMMON_HIREDIS_SOURCE_FILES})
