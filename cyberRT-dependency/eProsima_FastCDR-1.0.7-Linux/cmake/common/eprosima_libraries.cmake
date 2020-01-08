# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

macro(eprosima_find_package package)

    if(NOT ${package}_FOUND AND NOT (EPROSIMA_INSTALLER AND (MSVC OR MSVC_IDE)))

        # Parse arguments.
        set(options REQUIRED)
        cmake_parse_arguments(FIND "${options}" "" "" ${ARGN})

        option(THIRDPARTY_${package} "Activate the use of internal thirdparty ${package}" OFF)

        find_package(${package} QUIET)
        if(NOT ${package}_FOUND AND (THIRDPARTY OR THIRDPARTY_${package}))
            set(SUBDIRECTORY_EXIST TRUE)
            if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/${package}/CMakeLists.txt")
                execute_process(
                    COMMAND git submodule update --recursive --init "thirdparty/${package}"
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    RESULT_VARIABLE EXECUTE_RESULT
                    )
                if(NOT EXECUTE_RESULT EQUAL 0)
                    set(SUBDIRECTORY_EXIST FALSE)
                    message(WARNING "Cannot configure Git submodule ${package}")
                endif()
            endif()

            if(SUBDIRECTORY_EXIST)
                add_subdirectory(${PROJECT_SOURCE_DIR}/thirdparty/${package})
                set(${package}_FOUND TRUE)
                if(NOT IS_TOP_LEVEL)
                    set(${package}_FOUND TRUE PARENT_SCOPE)
                endif()
            endif()
        endif()

        if(${package}_FOUND)
            message(STATUS "${package} library found...")
        elseif(${FIND_REQUIRED})
            message(FATAL_ERROR "${package} library not found...")
        else()
            message(STATUS "${package} library not found...")
        endif()
    endif()
endmacro()

macro(eprosima_find_thirdparty package thirdparty_name)
    if(NOT (EPROSIMA_INSTALLER AND (MSVC OR MSVC_IDE)))

        option(THIRDPARTY_${package} "Activate the use of internal thirdparty ${package}" OFF)

        if(THIRDPARTY OR THIRDPARTY_${package})
            execute_process(
                COMMAND git submodule update --recursive --init "thirdparty/${thirdparty_name}"
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                RESULT_VARIABLE EXECUTE_RESULT
                )

            if(EXECUTE_RESULT EQUAL 0)
            else()
                message(FATAL_ERROR "Cannot configure Git submodule ${package}")
            endif()
        endif()

        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PROJECT_SOURCE_DIR}/thirdparty/${thirdparty_name})
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PROJECT_SOURCE_DIR}/thirdparty/${thirdparty_name}/${thirdparty_name})
        find_package(${package} REQUIRED QUIET)

    endif()
endmacro()
