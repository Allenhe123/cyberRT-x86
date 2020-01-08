find_package(fastrtps REQUIRED CONFIG)
message(STATUS "Found fastrtps ${fastrtps_FIND_VERSION}")
message(STATUS "${fastrtps_LIB_DIR}, ${fastrtps_INCLUDE_DIR}")
include_directories(${fastrtps_INCLUDE_DIR})
link_directories(${fastrtps_LIB_DIR})

find_package(fastcdr REQUIRED CONFIG)
message(STATUS "Found fastcdr ${fastcdr_FIND_VERSION}")
message(STATUS "${fastcdr_LIB_DIR}, ${fastcdr_INCLUDE_DIR}")
include_directories(${fastcdr_INCLUDE_DIR})
link_directories(${fastcdr_LIB_DIR})

find_package(Protobuf REQUIRED)
message(STATUS "Found Protobuf ${PROTOBUF_INCLUDE_DIR} ${PROTOBUF_LIBRARIES}")
include_directories(${PROTOBUF_INCLUDE_DIR})
link_directories(${PROTOBUF_LIB_DIR})

find_package(Poco REQUIRED COMPONENTS Foundation CONFIG)
message(STATUS "Found Poco: ${Poco_LIBRARIES}")
