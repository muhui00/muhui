cmake_minimum_required(VERSION 3.16.3)
project(mumu)

include (cmake/utils.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wunused-const-variable -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(mumu)

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(LIB_SRC
    mumu/address.cc
    mumu/log.cc
    mumu/util.cc
    mumu/mutex.cc
    mumu/config.cc
    mumu/thread.cc
    mumu/fiber.cc
    mumu/scheduler.cc
    mumu/iomanager.cc
    mumu/timer.cc
    mumu/hook.cc
    mumu/fd_manager.cc
    mumu/socket.cc
    mumu/bytearray.cc
    mumu/stream.cc
    mumu/tcp_server.cc
    mumu/http/http.cc
    mumu/http/http11_parser.rl.cc
    mumu/http/httpclient_parser.rl.cc
    mumu/http/http_parser.cc
    mumu/http/http_session.cc
    mumu/http/http_server.cc
    mumu/http/servlet.cc
    mumu/streams/socket_stream.cc
    mumu/util/json_util.cc
    mumu/util/hash_util.cc
    )

add_library(mumu SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(mumu)

set(LIBS
    mumu
    pthread
    yaml-cpp
    dl
    jsoncpp
    protobuf
    ${OPENSSL_LIBRARIES}
    )

function(muhui_add_executable targetname srcs depends libs)
    add_executable(${targetname} ${srcs})
    add_dependencies(${targetname} ${depends})
    force_redefine_file_macro_for_sources(${targetname})
    target_link_libraries(${targetname} ${libs})
endfunction()

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config mumu)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIBS})

add_executable(test tests/test.cc)
add_dependencies(test mumu)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread mumu)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util mumu)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber mumu)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler mumu)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager mumu)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook mumu)
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIBS})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address mumu)
force_redefine_file_macro_for_sources(test_address)
target_link_libraries(test_address ${LIBS})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket mumu)
force_redefine_file_macro_for_sources(test_socket)
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray mumu)
force_redefine_file_macro_for_sources(test_bytearray)
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http mumu)
force_redefine_file_macro_for_sources(test_http)
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser mumu)
force_redefine_file_macro_for_sources(test_http_parser)
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server mumu)
force_redefine_file_macro_for_sources(test_tcp_server)
target_link_libraries(test_tcp_server ${LIBS})

add_executable(echo_tcp_server examples/echo_tcp_server.cc)
add_dependencies(echo_tcp_server mumu)
force_redefine_file_macro_for_sources(echo_tcp_server)
target_link_libraries(echo_tcp_server ${LIBS})

muhui_add_executable(test_http_server "tests/test_http_server.cc" mumu "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
