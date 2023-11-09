#---------------------------------------------------------------------------//
# Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#---------------------------------------------------------------------------//

include(CMTest)

set(BUILD_WITH_UNIT_TEST_SMP
    2
    CACHE
    STRING
    "Run unit tests with this many cores.")

#
# Define a new unit test with the given name.
#
# actor_add_test(name
#   [KIND {ACTOR,BOOST,CUSTOM}]
#   [SOURCES source1 source2 ... sourcen]
#   [WORKING_DIRECTORY dir]
#   [LIBRARIES library1 library2 ... libraryn]
#   [RUN_ARGS arg1 arg2 ... argn])
#
# There are three kinds of test we support (the KIND parameter):
#
# - ACTOR: Unit tests which use macros like `ACTOR_TEST_CASE`
# - BOOST: Unit tests which use macros like `BOOST_AUTO_TEST_CASE`
# - CUSTOM: Custom tests which need to be specified
#
# ACTOR and BOOST tests will have their output saved for interpretation by the Jenkins continuous integration service
# if this is configured for the build.
#
# KIND can be omitted, in which case it is assumed to be ACTOR.
#
# If SOURCES is provided, then the test files are first compiled into an executable which has the same name as the test
# but with a suffix ("_test").
#
# WORKING_DIRECTORY can be optionally provided to choose where the test is executed.
#
# If LIBRARIES is provided along with SOURCES, then the executable is additionally linked with these libraries.
#
# RUN_ARGS are optional additional arguments to pass to the executable. For ACTOR tests, these come after `--`. For
# CUSTOM tests with no SOURCES, this parameter can be used to specify the executable name as well as its arguments since
# no executable is compiled.
#
function(actor_add_test name)
    set(test_kinds
        ACTOR
        BOOST
        CUSTOM)

    cmake_parse_arguments(parsed_args
                          ""
                          "WORKING_DIRECTORY;KIND"
                          "RUN_ARGS;SOURCES;LIBRARIES;DEPENDS"
                          ${ARGN})

    if(NOT parsed_args_KIND)
        set(parsed_args_KIND ACTOR)
    elseif(NOT (parsed_args_KIND IN_LIST test_kinds))
        message(FATAL_ERROR "Invalid test kind. KIND must be one of ${test_kinds}")
    endif()

    if(parsed_args_SOURCES)
        #
        # Each kind of test must populate the `args` and `libraries` lists.
        #

        set(libraries "${parsed_args_LIBRARIES}")

        set(args "")
        if(parsed_args_KIND STREQUAL "ACTOR")
            list(APPEND libraries
                 ${CMAKE_WORKSPACE_NAME}::core
                 ${CMAKE_WORKSPACE_NAME}::network
                 ${CMAKE_WORKSPACE_NAME}::testing)

            list(APPEND args -- -c ${BUILD_WITH_UNIT_TEST_SMP})
        elseif(parsed_args_KIND STREQUAL "BOOST")
            list(APPEND libraries
                 Boost::unit_test_framework
                 ${CMAKE_WORKSPACE_NAME}::core
                 ${CMAKE_WORKSPACE_NAME}::network
                 ${CMAKE_WORKSPACE_NAME}::testing)
        endif()

        if(NOT (UNIX AND (CMAKE_SYSTEM_NAME STREQUAL "Linux")))
            list(APPEND libraries epoll-shim)
        endif()

        list(APPEND args ${parsed_args_RUN_ARGS})

        cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                               ${libraries}
                               ${Boost_LIBRARIES})

        cm_test(NAME ${CMAKE_WORKSPACE_NAME}_${name}_test SOURCES ${parsed_args_SOURCES})

        target_compile_definitions(${CMAKE_WORKSPACE_NAME}_${name}_test PRIVATE ACTOR_TESTING_MAIN)

        if((BUILD_WITH_STACK_GUARDS STREQUAL "ON") OR
        ((BUILD_WITH_STACK_GUARDS STREQUAL "DEFAULT") AND
        (CMAKE_BUILD_TYPE IN_LIST ACTOR_STACK_GUARD_MODES)))
            target_compile_definitions(${CMAKE_WORKSPACE_NAME}_${name}_test
                                       PRIVATE
                                       ACTOR_THREAD_STACK_GUARDS)
        endif()

        target_include_directories(${CMAKE_WORKSPACE_NAME}_${name}_test PRIVATE
                                   ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${BUILD_WITH_SOURCE_DIR}/src)

        set_target_properties(${CMAKE_WORKSPACE_NAME}_${name}_test
                              PROPERTIES
                              CXX_STANDARD 17
                              CXX_STANDARD_REQUIRED TRUE)

        target_compile_options(${CMAKE_WORKSPACE_NAME}_${name}_test PRIVATE
                               ${${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS})
    else()
        if(NOT (parsed_args_KIND STREQUAL "CUSTOM"))
            message(FATAL_ERROR "SOURCES are required for ${parsed_args_KIND} tests")
        endif()
    endif()
endfunction()

#
# Define a new custom unit test whose entry point is a Actor application.
#
# actor_add_app_test(name
#   [SOURCES source1 source2 ... sourcen]
#   [LIBRARIES library1 library2 ... libraryn]
#   [RUN_ARGS arg1 arg2 ... argn])
#
# These kinds of tests are structured like Actor applications.
#
# These tests always link against `actor_private` and are always invoked with
# `-c ${BUILD_WITH_UNIT_TEST_SMP}`.
#
function(actor_add_app_test name)
    cmake_parse_arguments(parsed_args
                          ""
                          ""
                          "RUN_ARGS;SOURCES;LIBRARIES"
                          ${ARGN})

    actor_add_test(${name}
                   KIND CUSTOM
                   SOURCES ${parsed_args_SOURCES}
                   LIBRARIES
                   ${CMAKE_WORKSPACE_NAME}::core
                   ${CMAKE_WORKSPACE_NAME}::network
                   ${CMAKE_WORKSPACE_NAME}::testing
                   ${parsed_args_LIBRARIES}
                   RUN_ARGS
                   -c ${BUILD_WITH_UNIT_TEST_SMP}
                   ${parsed_args_RUN_ARGS})
endfunction()

function(prepend_each var prefix)
    set(result "")

    foreach(x ${ARGN})
        list(APPEND result ${prefix}/${x})
    endforeach()

    set(${var} ${result} PARENT_SCOPE)
endfunction()

if(NOT ACTOR_EXECUTE_ONLY_FAST_TESTS)
    set(allocator_test_args "")
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(allocator_test_args --iterations 5)
    else()
        set(allocator_test_args --time 0.1)
    endif()
endif()

actor_add_test(connect
               SOURCES connect_test.cc)

actor_add_test(dns
               SOURCES dns_test.cc)

actor_add_test(httpd
               SOURCES
               httpd_test.cc
               loopback_socket.hh)

actor_add_test(ipv6
               SOURCES ipv6_test.cc)

actor_add_test(network_interface
               SOURCES network_interface_test.cc)

actor_add_test(json_formatter
               SOURCES json_formatter_test.cc)

actor_add_test(net_config
               KIND BOOST
               SOURCES net_config_test.cc)

actor_add_test(output_stream
               SOURCES output_stream_test.cc)

actor_add_test(packet
               KIND BOOST
               SOURCES packet_test.cc)

actor_add_test(request_parser
               SOURCES request_parser_test.cc)

actor_add_test(rpc
               SOURCES
               loopback_socket.hh
               rpc_test.cc)

actor_add_app_test(socket
                   SOURCES socket_test.cc)

function(actor_add_certgen name)
    cmake_parse_arguments(CERT
                          ""
                          "SUBJECT;SERVER;NAME;DOMAIN;COMMON;LOCALITY;ORG;WIDTH;STATE;COUNTRY;UNIT;EMAIL;DAYS;ALG"
                          "ALG_OPTS"
                          ${ARGN}
                          )

    if(NOT CERT_SERVER)
        execute_process(COMMAND hostname
                        RESULT_VARIABLE CERT_SERVER
                        )
    endif()
    if(NOT CERT_DOMAIN)
        execute_process(COMMAND dnsdomainname
                        RESULT_VARIABLE CERT_DOMAIN
                        )
    endif()
    if(NOT CERT_NAME)
        set(CERT_NAME ${CERT_SERVER})
    endif()
    if(NOT CERT_COUNTRY)
        set(CERT_COUNTRY SE)
    endif()
    if(NOT CERT_STATE)
        set(CERT_STATE Stockholm)
    endif()
    if(NOT CERT_LOCALITY)
        set(CERT_LOCALITY ${CERT_STATE})
    endif()
    if(NOT CERT_ORG)
        set(CERT_ORG ${CERT_DOMAIN})
    endif()
    if(NOT CERT_UNIT)
        set(CERT_UNIT ${CERT_DOMAIN})
    endif()
    if(NOT CERT_COMMON)
        set(CERT_COMMON ${CERT_SERVER}.${CERT_DOMAIN})
    endif()
    if(NOT CERT_EMAIL)
        set(CERT_EMAIL postmaster@${CERT_DOMAIN})
    endif()
    if(NOT CERT_WIDTH)
        set(CERT_WIDTH 4096)
    endif()
    if(NOT CERT_DAYS)
        set(CERT_DAYS 3650)
    endif()
    if((NOT CERT_ALG) AND (NOT CERT_ALG_OPTS))
        set(CERT_ALG_OPTS -pkeyopt rsa_keygen_bits:${CERT_WIDTH})
    endif()
    if(NOT CERT_ALG)
        set(CERT_ALG RSA)
    endif()

    set(CERT_PRIVKEY ${CERT_NAME}.key)
    set(CERT_REQ ${CERT_NAME}.csr)
    set(CERT_CERT ${CERT_NAME}.crt)

    set(CERT_CAPRIVKEY ca${CERT_NAME}.key)
    set(CERT_CAROOT ca${CERT_NAME}.pem)

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cert.cfg.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/${CERT_NAME}.cfg"
                   )

    find_program(OPENSSL openssl)

    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CERT_PRIVKEY}"
                       COMMAND ${OPENSSL} genpkey -out ${CERT_PRIVKEY} -algorithm ${CERT_ALG} ${CERT_ALG_OPTS}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       )
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CERT_REQ}"
                       COMMAND ${OPENSSL} req -new -key ${CERT_PRIVKEY} -out ${CERT_REQ} -config ${CERT_NAME}.cfg
                       DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${CERT_PRIVKEY}"
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       )

    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CERT_CAPRIVKEY}"
                       COMMAND ${OPENSSL} genpkey -out ${CERT_CAPRIVKEY} -algorithm ${CERT_ALG} ${CERT_ALG_OPTS}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       )
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CERT_CAROOT}"
                       COMMAND ${OPENSSL} req -x509 -new -nodes -key ${CERT_CAPRIVKEY} -days ${CERT_DAYS} -config ${CERT_NAME}.cfg -out ${CERT_CAROOT}
                       DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${CERT_CAPRIVKEY}"
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       )

    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CERT_CERT}"
                       COMMAND ${OPENSSL} x509 -req -in ${CERT_REQ} -CA ${CERT_CAROOT} -CAkey ${CERT_CAPRIVKEY} -CAcreateserial -out ${CERT_CERT} -days ${CERT_DAYS}
                       DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${CERT_REQ}" "${CMAKE_CURRENT_BINARY_DIR}/${CERT_CAROOT}"
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       )

    add_custom_target(${name} DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${CERT_CERT}")
endfunction()

actor_add_certgen(testcrt DOMAIN nil.foundation SERVER test)
actor_add_certgen(othercrt DOMAIN apa.org SERVER other)

set(tls_certificate_files tls-ca-bundle.pem)

prepend_each(
        in_tls_certificate_files
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${tls_certificate_files})

prepend_each(out_tls_certificate_files
             ${CMAKE_CURRENT_BINARY_DIR}/
             ${tls_certificate_files})

add_custom_command(
        DEPENDS ${in_tls_certificate_files}
        OUTPUT ${out_tls_certificate_files}
        COMMAND ${CMAKE_COMMAND} -E copy ${in_tls_certificate_files} ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(tls_files
                  DEPENDS ${out_tls_certificate_files}
                  )

if(NOT Boost_FILESYSTEM_FOUND)
    find_package(Boost 1.64.0 REQUIRED COMPONENTS filesystem)
endif()

actor_add_test(tls
               DEPENDS tls_files testcrt othercrt
               SOURCES tls_test.cc
               LIBRARIES ${Boost_LIBRARIES}
               WORKING_DIRECTORY ${BUILD_WITH_BINARY_DIR})

actor_add_test(unix_domain
               SOURCES unix_domain_test.cc)
