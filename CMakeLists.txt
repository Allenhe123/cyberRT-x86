cmake_minimum_required(VERSION 3.12)

include(GNUInstallDirs)

#project setting
project(cyber)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_INCLUDE_SYSTEM_FLAG_C "-I")
SET( CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-I" CACHE STRING "" FORCE )
SET( CMAKE_INCLUDE_SYSTEM_FLAG_CUDA "-I" CACHE STRING "" FORCE )
add_definitions(-std=c++11)

enable_language(ASM)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
	set(SWAP_SRCS "${PROJECT_SOURCE_DIR}/cyber/croutine/detail/swap_x86_64.S")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
	set(SWAP_SRCS "${PROJECT_SOURCE_DIR}/cyber/croutine/detail/swap_aarch64.S")
else()
	message(FATAL_ERROR "Can not set swap source file, please check")
	return()
endif()

#include dependencies
include(cmake/Dependencies.cmake)

#build proto files
add_subdirectory(cyber/proto)

#build googletest
add_subdirectory(googletest)

#collect source files
file(GLOB CYBER_SRCS 
	"${PROJECT_SOURCE_DIR}/cyber/base/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/blocker/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/class_loader/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/class_loader/*/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/common/*.cc" 
	"${PROJECT_SOURCE_DIR}/cyber/component/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/croutine/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/croutine/*/*.cc"
#	"${PROJECT_SOURCE_DIR}/cyber/croutine/detail/swap_aarch64.S"
	"${SWAP_SRCS}"
	"${PROJECT_SOURCE_DIR}/cyber/data/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/event/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/io/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/logger/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/message/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/node/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/parameter/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/record/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/record/*/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/scheduler/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/scheduler/*/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/service/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/service_discovery/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/service_discovery/*/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/task/*.cc" 
	"${PROJECT_SOURCE_DIR}/cyber/time/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/timer/*.cc"	
	"${PROJECT_SOURCE_DIR}/cyber/transport/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/transport/*/*.cc"
	"${PROJECT_SOURCE_DIR}/cyber/*.cc"
	)
set(CYBER_TEST_SRCS ${CYBER_SRCS})
list(FILTER CYBER_TEST_SRCS INCLUDE REGEX .*test[.]cc )
list(FILTER CYBER_SRCS EXCLUDE REGEX .*test[.]cc )

#include_directories
include_directories(${PROJECT_BINARY_DIR})

#build cyber library
add_library(cyber SHARED ${CYBER_SRCS})
target_link_libraries(cyber cyber_proto fastrtps fastcdr glog ${Poco_LIBRARIES} atomic uuid)

#build mainboard
file(GLOB CYBER_MAINBOARD_SRCS "${PROJECT_SOURCE_DIR}/cyber/mainboard/*.cc")
add_executable(mainboard ${CYBER_MAINBOARD_SRCS})
target_link_libraries(mainboard cyber gflags glog)
target_compile_options(mainboard PRIVATE -Og)

#build examples
add_subdirectory(cyber/examples)
add_subdirectory(cyber/examples/proto)

#build python wrapper
add_subdirectory(cyber/py_wrapper)

#build tools 
add_subdirectory(cyber/tools)

#build tests
FOREACH(TEST_FILE ${CYBER_TEST_SRCS})  
	string( REGEX MATCH "[A-Za-z0-9_]*[.]cc" FILE_NAME ${TEST_FILE} ) 
	string( REGEX MATCH "[A-Za-z0-9_]*" TARGET_NAME ${FILE_NAME})
	list(APPEND TEST_TARGETS ${TARGET_NAME})
	add_executable(${TARGET_NAME} ${TEST_FILE})
	target_link_libraries(${TARGET_NAME} cyber gflags glog gtest gmock_main)
	target_compile_options(${TARGET_NAME} PRIVATE -Og)
ENDFOREACH(TEST_FILE)

#set(CMAKE_INSTALL_BINDIR /home/allen/cyber-xavier/build/bin)
#set(CMAKE_INSTALL_LIBDIR /home/allen/cyber-xavier/build/lib)
#set(CMAKE_INSTALL_INCLUDEDIR /home/allen/cyber-xavier/build/include)

#install
install(TARGETS cyber mainboard 
 		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${TEST_TARGETS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber/test)
install(DIRECTORY cyber DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${PROJECT_BINARY_DIR}/cyber DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
install(DIRECTORY cyber/conf DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber)
install(FILES cyber/setup.bash DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber)
install(DIRECTORY cyber/python DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber)
