./channel_buffer_test
./atomic_hash_map_test
./atomic_rw_lock_test
./node_manager_test
./all_test.sh
./time_test
./logger_test
./raw_message_test
./object_pool_test
./timing_wheel_test
./file_test
./parameter_server_test
./timer_component_test
./channel_manager_test
./common_test
./record_viewer_test
./record_reader_test
./data_dispatcher_test
./component_test
./for_each_test
./message_header_test
./dispatcher_test
./shm_transceiver_test&
sleep 2
ps|grep _test|cut -c 1-5|xargs kill
./protobuf_factory_test
./writer_reader_test
./macros_test
./blocker_manager_test
./task_test
./rtps_dispatcher_test&
sleep 3
ps|grep _test|cut -c 1-5|xargs kill
./rtps_test
./intra_dispatcher_test
./graph_test
./log_test
./warehouse_test
./role_test
./environment_test
./message_test
./parameter_test
./timer_manager_test&
sleep 1
ps|grep _test|cut -c 1-5|xargs kill
./node_test
./data_visitor_test
./scheduler_test
./intra_transceiver_test
./signal_test
./hybrid_transceiver_test&
sleep 3
ps|grep _test|cut -c 1-5|xargs kill
./croutine_test
./cache_buffer_test
./service_manager_test
./message_traits_test
./transport_test&
sleep 1
ps|grep _test|cut -c 1-5|xargs kill
./parameter_client_test
./scheduler_policy_test
./bounded_queue_test
./blocker_test
./duration_test
./shm_dispatcher_test&
sleep 4
ps|grep _test|cut -c 1-5|xargs kill
./poller_test&
sleep 1
ps|grep _test|cut -c 1-5|xargs kill
./log_file_object_test
./rtps_transceiver_test&
sleep 2
ps|grep _test|cut -c 1-5|xargs kill
./topology_manager_test
./record_file_test
