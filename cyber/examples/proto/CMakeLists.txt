project(cyber_example_proto)

file(GLOB CYBER_PROTO_SRCS *.proto)
PROTOBUF_GENERATE_CPP(	PROTO_SRCS PROTO_HDRS ${CYBER_PROTO_SRCS})

include_directories(${cyber_BINARY_DIR})
add_library(cyber_example_proto SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(cyber_example_proto ${Protobuf_LIBRARIES})

#set(CMAKE_INSTALL_LIBDIR /home/allen/cyber-a500/build/lib)
#set(CMAKE_INSTALL_FILEDIR /home/allen/cyber-a500/build/examples/proto)
set(CMAKE_INSTALL_FILEDIR ${CMAKE_INSTALL_INCLUDEDIR})
message(STATUS "CMAKE_INSTALL_FILEDIR: ${CMAKE_INSTALL_FILEDIR}")

install(TARGETS cyber_example_proto LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROTO_HDRS}  DESTINATION ${CMAKE_INSTALL_FILEDIR})
