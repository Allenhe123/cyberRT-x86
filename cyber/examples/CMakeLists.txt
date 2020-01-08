project(cyber_example)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${cyber_SOURCE_DIR})
include_directories(${cyber_BINARY_DIR})
add_compile_options(-Og)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/examples.proto)

add_executable(talker talker.cc ${PROTO_SRCS} )
add_executable(listener listener.cc ${PROTO_SRCS})
add_executable(paramserver paramserver.cc)
add_executable(service service.cc ${PROTO_SRCS})
add_executable(record record.cc)

target_link_libraries(talker cyber gflags glog)
target_link_libraries(listener cyber gflags glog)
target_link_libraries(paramserver cyber gflags glog)
target_link_libraries(service cyber gflags glog)
target_link_libraries(record cyber cyber_proto gflags glog)

add_library(common_component_example SHARED common_component_example/common_component_example.cc ${PROTO_SRCS})
add_library(timer_component_example SHARED timer_component_example/timer_component_example.cc ${PROTO_SRCS})
target_link_libraries(common_component_example cyber)
target_link_libraries(timer_component_example cyber)

file(GLOB EXAMPLE_FILES "*/*.dag" "*/*.launch")
install(TARGETS common_component_example timer_component_example talker listener paramserver service record
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/examples
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber/examples)
install(FILES ${EXAMPLE_FILES} DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/examples)
