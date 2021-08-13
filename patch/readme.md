## update yo 6.0.0 :
1. cd cyber/proto directory, change the file path of all "import xxx.proto"
   grep -nHR 'import' cyber/proto

2. vim cyber/proto/unit_test.proto, add below message:
message Needata {
    optional uint64 timestamp = 1;
    optional uint32 seq = 2;
    optional uint32 id = 3;
    optional bytes content = 4;
    optional bytes data1 = 5;
    optional bytes data2 = 6;
    optional bytes data3 = 7;
    optional bytes data4 = 8;
    optional bytes data5 = 9;
    optional bytes data6 = 10;
    optional bytes data7 = 11;
    optional bytes data8 = 12;
    optional bytes data9 = 13;
};

3. cd cyber/examples, vim CMakeLists.txt, comment below lines:
add_executable(cyber_bridge cyber_bridge.cc ${PROTO_SRCS})
add_executable(talker_cyber_bridge talker_cyber_bridge.cc ${PROTO_SRCS})

target_link_libraries(cyber_bridge cyber gflags glog)
target_link_libraries(talker_cyber_bridge cyber gflags glog)
