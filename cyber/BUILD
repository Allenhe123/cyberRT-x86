load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//tools:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cyber",
    linkstatic = False,
    deps = [
        "//cyber:cyber_core",
    ],
)

cc_binary(
    name = "mainboard",
    srcs = [
        "mainboard/mainboard.cc",
        "mainboard/module_argument.cc",
        "mainboard/module_argument.h",
        "mainboard/module_controller.cc",
        "mainboard/module_controller.h",
    ],
    linkopts = ["-pthread"],
    linkstatic = False,
    deps = [
        ":cyber_core",
        "//cyber/proto:dag_conf_cc_proto",
    ],
)

cc_library(
    name = "binary",
    hdrs = ["binary.h"],
    srcs = ["binary.cc"],
)

cc_library(
    name = "state",
    srcs = ["state.cc"],
    hdrs = ["state.h"],
    deps = [
        "//cyber/common",
    ],
)

cc_library(
    name = "init",
    srcs = ["init.cc"],
    hdrs = ["init.h"],
    deps = [
        "//cyber:binary",
        "//cyber:state",
        "//cyber/common:file",
        "//cyber/logger:async_logger",
        "//cyber/node",
        "//cyber/proto:clock_cc_proto",
        "//cyber/sysmo",
        "//cyber/time:clock",
        "//cyber/timer:timing_wheel",
    ],
)

cc_library(
    name = "cyber_core",
    srcs = ["cyber.cc"],
    hdrs = ["cyber.h"],
    linkopts = ["-lrt"],
    deps = [
        "//cyber:binary",
        "//cyber:init",
        "//cyber:state",
        "//cyber/base",
        "//cyber/blocker:blocker_manager",
        "//cyber/class_loader",
        "//cyber/class_loader:class_loader_manager",
        "//cyber/common",
        "//cyber/component",
        "//cyber/component:timer_component",
        "//cyber/croutine",
        "//cyber/data",
        "//cyber/event:perf_event_cache",
        "//cyber/io",
        "//cyber/logger",
        "//cyber/logger:async_logger",
        "//cyber/message:message_traits",
        "//cyber/message:protobuf_traits",
        "//cyber/message:py_message_traits",
        "//cyber/message:raw_message_traits",
        "//cyber/node",
        "//cyber/parameter:parameter_client",
        "//cyber/parameter:parameter_server",
        "//cyber/proto:run_mode_conf_cc_proto",
        "//cyber/record",
        "//cyber/scheduler",
        "//cyber/scheduler:scheduler_factory",
        "//cyber/service",
        "//cyber/service:client",
        "//cyber/service_discovery:topology_manager",
        "//cyber/sysmo",
        "//cyber/task",
        "//cyber/time",
        "//cyber/time:clock",
        "//cyber/time:duration",
        "//cyber/time:rate",
        "//cyber/timer",
        "//cyber/transport",
        "//cyber/transport:participant",
        "//cyber/transport:sub_listener",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@fastrtps",
        "@uuid",
    ],
)

filegroup(
    name = "cyber_conf",
    srcs = glob([
        "conf/*.conf",
    ]),
)

cpplint()
