project(io_example)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${cyber_SOURCE_DIR})
include_directories(${cyber_BINARY_DIR})
add_compile_options(-Og)

add_executable(tcp_echo_client tcp_echo_client.cc)
add_executable(tcp_echo_server tcp_echo_server.cc)
add_executable(udp_echo_client udp_echo_client.cc)
add_executable(udp_echo_server udp_echo_server.cc)

target_link_libraries(tcp_echo_client cyber gflags glog)
target_link_libraries(tcp_echo_server cyber gflags glog)
target_link_libraries(udp_echo_client cyber gflags glog)
target_link_libraries(udp_echo_server cyber gflags glog)

install(TARGETS tcp_echo_client tcp_echo_server udp_echo_client udp_echo_server 
	DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber/io/example)
