project(cyber_proto)

file(GLOB CYBER_PROTO_SRCS *.proto)
PROTOBUF_GENERATE_CPP(	PROTO_SRCS PROTO_HDRS ${CYBER_PROTO_SRCS})

include_directories(${cyber_BINARY_DIR})
message(STATUS "cyber_BINARY_DIR: " ${cyber_BINARY_DIR})

add_library(cyber_proto SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(cyber_proto ${Protobuf_LIBRARIES})
message(STATUS "protobuf_library: " ${Protobuf_LIBRARIES})

#set(CMAKE_INSTALL_LIBDIR /home/allen/cyber-a500/build/lib)

install(TARGETS cyber_proto LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
