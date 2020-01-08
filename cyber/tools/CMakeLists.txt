project(cyber_tools)

#include cyber directories
include_directories(${cyber_BINARY_DIR})
include_directories(${cyber_SOURCE_DIR})
include_directories(${cyber_BINARY_DIR})

#install cyber_launch
install(DIRECTORY cyber_launch DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber/tools)

#build cyber_monitor
file(GLOB CYBER_MONITOR_SRCS "${PROJECT_SOURCE_DIR}/cyber_monitor/*.cc")
add_executable(cyber_monitor ${CYBER_MONITOR_SRCS})
target_link_libraries(cyber_monitor cyber gflags glog pthread ncurses)
install(TARGETS cyber_monitor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber/tools/cyber_monitor)

#build cyber_recorder
file(GLOB CYBER_RECORDER_SRCS "${PROJECT_SOURCE_DIR}/cyber_recorder/*.cc" "${PROJECT_SOURCE_DIR}/cyber_recorder/player/*.cc")
add_executable(cyber_recorder ${CYBER_RECORDER_SRCS})
target_link_libraries(cyber_recorder cyber gflags glog)
install(TARGETS cyber_recorder RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber/tools/cyber_recorder)

#install cyber_tools_auto_complete.bash
install(FILES cyber_tools_auto_complete.bash DESTINATION ${CMAKE_INSTALL_BINDIR}/cyber/tools)
