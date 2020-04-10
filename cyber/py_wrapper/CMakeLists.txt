project(cyber_py_wrapper)

#include cyber directories
include_directories(${cyber_BINARY_DIR})
include_directories(${cyber_SOURCE_DIR})
include_directories(${cyber_BINARY_DIR})

#build _cyber_init.so
#add_library(_cyber_init SHARED "cyber_init_wrap.cc")
#target_link_libraries(_cyber_init cyber)
#install(CODE "file(RENAME ${cyber_BINARY_DIR}/lib_cyber_init.so ${cyber_BINARY_DIR}/_cyber_init.so)")
#install(FILES "${cyber_BINARY_DIR}/_cyber_init.so" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/py_wrapper)

#build _cyber_node.so
#add_library(_cyber_node SHARED "cyber_node_wrap.cc")
#target_link_libraries(_cyber_node cyber)
#install(CODE "file(RENAME ${cyber_BINARY_DIR}/lib_cyber_node.so ${cyber_BINARY_DIR}/_cyber_node.so)")
#install(FILES "${cyber_BINARY_DIR}/_cyber_node.so" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/py_wrapper)

#build _cyber_record.so
#add_library(_cyber_record SHARED "cyber_record_wrap.cc")
#target_link_libraries(_cyber_record cyber)
#install(CODE "file(RENAME ${cyber_BINARY_DIR}/lib_cyber_record.so ${cyber_BINARY_DIR}/_cyber_record.so)")
#install(FILES "${cyber_BINARY_DIR}/_cyber_record.so" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/py_wrapper)

#build _cyber_time.so
#add_library(_cyber_time SHARED "cyber_record_wrap.cc")
#target_link_libraries(_cyber_time cyber)
#install(CODE "file(RENAME ${cyber_BINARY_DIR}/lib_cyber_time.so ${cyber_BINARY_DIR}/_cyber_time.so)")
#install(FILES "${cyber_BINARY_DIR}/_cyber_time.so" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/py_wrapper)

#build _cyber_timer.so
#add_library(_cyber_timer SHARED "cyber_record_wrap.cc")
#target_link_libraries(_cyber_timer cyber)
#install(CODE "file(RENAME ${cyber_BINARY_DIR}/lib_cyber_timer.so ${cyber_BINARY_DIR}/_cyber_timer.so)")
#install(FILES "${cyber_BINARY_DIR}/_cyber_timer.so" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/py_wrapper)

#build _cyber_topology_manager.so
find_package(Python2 COMPONENTS Development)
include_directories(${Python2_INCLUDE_DIRS})
#add_library(_cyber_topology_manager SHARED "cyber_topology_manager_wrap.cc")
#target_link_libraries(_cyber_topology_manager cyber ${Python2_LIBRARIES})
#install(CODE "file(RENAME ${cyber_BINARY_DIR}/lib_cyber_topology_manager.so ${cyber_BINARY_DIR}/_cyber_topology_manager.so)")
#install(FILES "${cyber_BINARY_DIR}/_cyber_topology_manager.so" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cyber/py_wrapper)
