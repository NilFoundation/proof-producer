cmake_minimum_required(VERSION 3.12.4)

cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0057 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake"
     "${CMAKE_CURRENT_LIST_DIR}/cmake/packages"
     "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/share/modules/cmake")

cm_find_package(CM)

include(CMConfig)
include(CMDeploy)
include(CMSetupVersion)
include(ActorDependencies)
include(ExternalProject)

if(NOT CMAKE_WORKSPACE_NAME OR NOT ("${CMAKE_WORKSPACE_NAME}" STREQUAL "actor"))
    cm_workspace(actor)
endif()

macro(cm_find_package NAME)
    if(NOT "${NAME}" MATCHES "^${CMAKE_WORKSPACE_NAME}_.*$" AND NOT "${NAME}" STREQUAL CM)
        find_package(${ARGV})
    else()
        set(${ARGV0}_FOUND TRUE CACHE BOOL "")
    endif()
endmacro()

cm_project(core WORKSPACE_NAME ${CMAKE_WORKSPACE_NAME} LANGUAGES CXX)

cm_setup_version(VERSION 0.1.0 PREFIX ${CMAKE_WORKSPACE_NAME})

# This variable impacts the way DPDK is configured by cmake-cooking (if DPDK is enabled), so its definition needs to
# come before PROJECT.
set(BUILD_WITH_DPDK_MACHINE
    "native"
    CACHE
    STRING
    "Configure DPDK for this processor architecture (if `BUILD_WITH_DPDK` is enabled). A DPDK code name abbreviation (e.g., ivb)")

option(BUILD_WITH_ALLOC_FAILURE_INJECTION "Enable failure injection into the allocator." FALSE)
option(BUILD_WITH_TASK_BACKTRACE "Collect backtrace at deferring points." FALSE)
option(BUILD_WITH_DEBUG_ALLOCATIONS "For now just writes 0xab to newly allocated memory" FALSE)
option(BUILD_WITH_SSTRING "Use own string implementation" TRUE)

set(BUILD_WITH_API_LEVEL
    "6"
    CACHE
    STRING
    "Actor compatibility API level (2=server_socket::accept() returns accept_result, 3=make_file_output_stream(), make_file_data_sink() returns future<...>, 4=when_all_succeed returns future<std::tuple<>>, 5=future<T>::get() returns T&&), 6=future is not variadic")

set_property(CACHE BUILD_WITH_API_LEVEL
             PROPERTY
             STRINGS 2 3 4 5 6)

#
# Add a dev build type.
#
# All pre-defined build modes include optimizations or debug info,
# which make them slow to build. The dev build mode is intended for
# fast build/test iteration.
#

set(CMAKE_CXX_FLAGS_DEV
    "-O1"
    CACHE
    STRING
    "Flags used by the C++ compiler during dev builds."
    FORCE)

set(CMAKE_C_FLAGS_DEV
    "-O1"
    CACHE
    STRING
    "Flags used by the C compiler during dev builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV
    ""
    CACHE
    STRING
    "Flags used for linking binaries during dev builds."
    FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_DEV
    ""
    CACHE
    STRING
    "Flags used by the shared libraries linker during builds."
    FORCE)

mark_as_advanced(
        CMAKE_CXX_FLAGS_DEV
        CMAKE_C_FLAGS_DEV
        CMAKE_EXE_LINKER_FLAGS_DEV
        CMAKE_SHARED_LINKER_FLAGS_DEV)

set(CMAKE_CXX_FLAGS_SANITIZE
    "-Os -g"
    CACHE
    STRING
    "Flags used by the C++ compiler during sanitize builds."
    FORCE)

set(CMAKE_C_FLAGS_SANITIZE
    "-Os -g"
    CACHE
    STRING
    "Flags used by the C compiler during sanitize builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_SANITIZE
    ""
    CACHE
    STRING
    "Flags used for linking binaries during sanitize builds."
    FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_SANITIZE
    ""
    CACHE
    STRING
    "Flags used by the shared libraries linker during sanitize builds."
    FORCE)

mark_as_advanced(CMAKE_CXX_FLAGS_SANITIZE
                 CMAKE_C_FLAGS_SANITIZE
                 CMAKE_EXE_LINKER_FLAGS_SANITIZE
                 CMAKE_SHARED_LINKER_FLAGS_SANITIZE)

set(CMAKE_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}"
    CACHE
    STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Dev Sanitize."
    FORCE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(BUILD_WITH_ALLOC_PAGE_SIZE
    ""
    CACHE
    STRING
    "Override the Actor allocator page size, in bytes.")

set(BUILD_WITH_CXX_DIALECT
    "c++20"
    CACHE
    STRING
    "Compile Actor according to the named C++ standard.")
if(BUILD_WITH_CXX_DIALECT)
    # extract CXX standard (14/17/...) from BUILD_WITH_CXX_DIALECT
    string(REGEX MATCH "[0-9]+" STANDARD ${BUILD_WITH_CXX_DIALECT})
    set(CMAKE_CXX_STANDARD ${STANDARD})
    set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
endif()

set(BUILD_WITH_CXX_FLAGS "" CACHE STRING
    "Semicolon-separated list of extra compilation flags for Actor itself.")

option(BUILD_WITH_DPDK "Enable DPDK support." FALSE)
option(BUILD_WITH_HWLOC "Enable hwloc support." TRUE)
option(BUILD_WITH_EXCEPTION_SCALABILITY_WORKAROUND
       "Override the dl_iterate_phdr symbol to workaround C++ exception scalability issues."
       TRUE)

set(BUILD_WITH_LD_FLAGS
    ""
    CACHE
    STRING
    "Semicolon-separated list of extra linking flags for Actor itself.")

option(BUILD_WITH_NUMA "Enable NUMA support." TRUE)
option(BUILD_WITH_COMPRESS_DEBUG "Compress debug info." TRUE)
option(BUILD_WITH_SPLIT_DWARF "Use split dwarf." FALSE)
option(BUILD_WITH_HEAP_PROFILING
       "Enable heap profiling. No effect when Actor is compiled with the default allocator."
       FALSE)

set(BUILD_WITH_TEST_TIMEOUT
    "300"
    CACHE
    STRING
    "Maximum allowed time for a test to run, in seconds.")

# We set the following environment variables
# * ASAN_OPTIONS=disable_coredump=0:abort_on_error=1:detect_stack_use_after_return=1
#   By default asan disables core dumps because they used to be
#   huge. This is no longer the case since the shadow memory is
#   excluded, so it is safe to enable them.
# * UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1
#   Fail the test if any undefined behavior is found and use abort
#   instead of exit. Using abort is what causes core dumps to be
#   produced.
# * BOOST_TEST_CATCH_SYSTEM_ERRORS=no
#   Normally the boost test library handles SIGABRT and prevents core
#   dumps from being produced.

# This works great with clang and gcc 10.2, but unfortunately not any
# previous gcc.
set(BUILD_WITH_USE_AFTER_RETURN "")
if((NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")) OR
(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.2))
    set(BUILD_WITH_USE_AFTER_RETURN ":detect_stack_use_after_return=1")
endif()

set(BUILD_WITH_TEST_ENVIRONMENT
    "ASAN_OPTIONS=disable_coredump=0:abort_on_error=1${BUILD_WITH_USE_AFTER_RETURN};UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1;BOOST_TEST_CATCH_SYSTEM_ERRORS=no"
    CACHE
    STRING
    "Environment variables for running tests")

option(BUILD_WITH_UNUSED_RESULT_ERROR
       "Make [[nodiscard]] violations an error (instead of a warning)."
       FALSE)

set(BUILD_WITH_STACK_GUARDS
    "DEFAULT"
    CACHE
    STRING
    "Enable stack guards. Can be TRUE, FALSE or DEFAULT (which enables it for non release builds)")

option(BUILD_WITH_SANITIZE
       "Enable ASAN and UBSAN. Can be TRUE, FALSE or DEFAULT (which enables it for Debug and Sanitize)"
       FALSE)

set(BUILD_WITH_DEBUG_SHARED_PTR
    "DEFAULT"
    CACHE
    STRING
    "Enable shared_ptr debugging. Can be TRUE, FALSE or DEFAULT (which enables it for Debug and Sanitize)")

#
# Useful (non-cache) variables.
#

set(BUILD_WITH_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_WITH_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(BUILD_WITH_GEN_BINARY_DIR ${BUILD_WITH_BINARY_DIR}/gen)

actor_find_dependencies()

if(BUILD_WITH_DPDK AND NOT dpdk_FOUND AND NOT TARGET dpdk)
    # Use the "native" profile that DPDK defines in `dpdk/config`, but in `dpdk_configure.cmake` we override
    # CONFIG_RTE_MACHINE with `ACTOR_DPDK_MACHINE`.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(dpdk_quadruple arm64-armv8a-linuxapp-gcc)
    else()
        set(dpdk_quadruple ${CMAKE_SYSTEM_PROCESSOR}-native-linuxapp-gcc)
    endif()

    set(dpdk_args
        # gcc 10 defaults to -fno-common, which dpdk is not prepared for
        "EXTRA_CFLAGS=-Wno-error -fcommon"
        O=<BINARY_DIR>
        DESTDIR=<INSTALL_DIR>
        T=${dpdk_quadruple})

    externalproject_add(dpdk
                        GIT_REPOSITORY git@github.com:nilfoundation/dpdk.git
                        DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/libs/dpdk
                        SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/dpdk
                        CONFIGURE_COMMAND
                        COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
                        make ${dpdk_args} config
                        COMMAND
                        ${CMAKE_COMMAND}
                        -DACTOR_DPDK_MACHINE=${ACTOR_DPDK_MACHINE}
                        -DACTOR_DPDK_CONFIG_FILE_IN=<BINARY_DIR>/.config
                        -DACTOR_DPDK_CONFIG_FILE_CHANGES=${CMAKE_CURRENT_SOURCE_DIR}/dpdk_config
                        -DACTOR_DPDK_CONFIG_FILE_OUT=<BINARY_DIR>/${dpdk_quadruple}/.config
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/dpdk_configure.cmake
                        BUILD_COMMAND ""
                        INSTALL_COMMAND
                        ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
                        ${make_command} ${dpdk_args} install)

    set(actor_dpdk_obj actor-dpdk.o)
endif()

find_package(Valgrind)
if(VALGRIND_FOUND)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS ACTOR_HAS_VALGRIND)
endif()

# Private build dependencies not visible to consumers
find_package(ragel 6.10 REQUIRED)
find_package(Threads REQUIRED)
find_package(PthreadSetName REQUIRED)

set(${CURRENT_PROJECT_NAME}_HEADERS
    include/nil/actor/core/abort_source.hh
    include/nil/actor/core/alien.hh
    include/nil/actor/core/align.hh
    include/nil/actor/core/aligned_buffer.hh
    include/nil/actor/core/app_template.hh
    include/nil/actor/core/apply.hh
    include/nil/actor/core/array_map.hh
    include/nil/actor/core/bitops.hh
    include/nil/actor/core/bitset-iter.hh
    include/nil/actor/core/byteorder.hh
    include/nil/actor/core/cacheline.hh
    include/nil/actor/core/checked_ptr.hh
    include/nil/actor/core/chunked_fifo.hh
    include/nil/actor/core/circular_buffer.hh
    include/nil/actor/core/circular_buffer_fixed_capacity.hh
    include/nil/actor/core/condition_variable.hh
    include/nil/actor/core/deleter.hh
    include/nil/actor/core/distributed.hh
    include/nil/actor/core/do_with.hh
    include/nil/actor/core/dpdk_rte.hh
    include/nil/actor/core/enum.hh
    include/nil/actor/core/execution_stage.hh
    include/nil/actor/core/exception_hacks.hh
    include/nil/actor/core/expiring_fifo.hh
    include/nil/actor/core/fair_queue.hh
    include/nil/actor/core/file.hh
    include/nil/actor/core/file-types.hh
    include/nil/actor/core/fsqual.hh
    include/nil/actor/core/fstream.hh
    include/nil/actor/core/function_traits.hh
    include/nil/actor/core/future-util.hh
    include/nil/actor/core/future.hh
    include/nil/actor/core/gate.hh
    include/nil/actor/core/iostream-impl.hh
    include/nil/actor/core/iostream.hh
    include/nil/actor/core/layered_file.hh
    include/nil/actor/core/loop.hh
    include/nil/actor/core/lowres_clock.hh
    include/nil/actor/core/manual_clock.hh
    include/nil/actor/core/map_reduce.hh
    include/nil/actor/core/memory.hh
    include/nil/actor/core/metrics.hh
    include/nil/actor/core/metrics_api.hh
    include/nil/actor/core/metrics_registration.hh
    include/nil/actor/core/metrics_types.hh
    include/nil/actor/core/network_stack_registry.hh
    include/nil/actor/core/pipe.hh
    include/nil/actor/core/posix.hh
    include/nil/actor/core/preempt.hh
    include/nil/actor/core/prefetch.hh
    include/nil/actor/core/print.hh
    include/nil/actor/core/queue.hh
    include/nil/actor/core/ragel.hh
    include/nil/actor/core/reactor.hh
    include/nil/actor/core/report_exception.hh
    include/nil/actor/core/resource.hh
    include/nil/actor/core/rwlock.hh
    include/nil/actor/core/scattered_message.hh
    include/nil/actor/core/scheduling.hh
    include/nil/actor/core/scollectd.hh
    include/nil/actor/core/scollectd_api.hh
    include/nil/actor/core/core.hh
    include/nil/actor/core/semaphore.hh
    include/nil/actor/core/sharded.hh
    include/nil/actor/core/shared_future.hh
    include/nil/actor/core/shared_mutex.hh
    include/nil/actor/core/shared_ptr.hh
    include/nil/actor/core/shared_ptr_debug_helper.hh
    include/nil/actor/core/shared_ptr_incomplete.hh
    include/nil/actor/core/simple_stream.hh
    include/nil/actor/core/slab.hh
    include/nil/actor/core/sleep.hh
    include/nil/actor/core/sstring.hh
    include/nil/actor/core/stall_sampler.hh
    include/nil/actor/core/stream.hh
    include/nil/actor/core/systemwide_memory_barrier.hh
    include/nil/actor/core/task.hh
    include/nil/actor/core/temporary_buffer.hh
    include/nil/actor/core/thread.hh
    include/nil/actor/core/thread_cputime_clock.hh
    include/nil/actor/core/thread_impl.hh
    include/nil/actor/core/timed_out_error.hh
    include/nil/actor/core/timer-set.hh
    include/nil/actor/core/timer.hh
    include/nil/actor/core/transfer.hh
    include/nil/actor/core/unaligned.hh
    include/nil/actor/core/units.hh
    include/nil/actor/core/vector-data-sink.hh
    include/nil/actor/core/weak_ptr.hh
    include/nil/actor/core/when_all.hh
    include/nil/actor/core/with_scheduling_group.hh
    include/nil/actor/core/with_timeout.hh

    include/nil/actor/detail/later.hh
    include/nil/actor/detail/alloc_failure_injector.hh
    include/nil/actor/detail/backtrace.hh
    include/nil/actor/detail/bool_class.hh
    include/nil/actor/detail/conversions.hh
    include/nil/actor/detail/defer.hh
    include/nil/actor/detail/eclipse.hh
    include/nil/actor/detail/function_input_iterator.hh
    include/nil/actor/detail/gcc6-concepts.hh
    include/nil/actor/detail/indirect.hh
    include/nil/actor/detail/is_smart_ptr.hh
    include/nil/actor/detail/lazy.hh
    include/nil/actor/detail/log-cli.hh
    include/nil/actor/detail/log-impl.hh
    include/nil/actor/detail/log.hh
    include/nil/actor/detail/noncopyable_function.hh
    include/nil/actor/detail/optimized_optional.hh
    include/nil/actor/detail/print_safe.hh
    include/nil/actor/detail/program-options.hh
    include/nil/actor/detail/read_first_line.hh
    include/nil/actor/detail/reference_wrapper.hh
    include/nil/actor/detail/spinlock.hh
    include/nil/actor/detail/std-compat.hh
    include/nil/actor/detail/transform_iterator.hh
    include/nil/actor/detail/tuple_utils.hh
    include/nil/actor/detail/variant_utils.hh)

# list cpp files excluding platform-dependent files
set(${CURRENT_PROJECT_NAME}_SOURCES
    src/core/alien.cc
    src/core/file.cc
    src/core/fair_queue.cc
    src/core/reactor_backend_epoll.cc
    src/core/reactor_backend_osv.cc
    src/core/reactor_backend_selector.cc
    src/core/thread_pool.cc
    src/core/app_template.cc
    src/core/dpdk_rte.cc
    src/core/exception_hacks.cc
    src/core/execution_stage.cc
    src/core/fsqual.cc
    src/core/fstream.cc
    src/core/future.cc
    src/core/future-util.cc
    src/core/memory.cc
    src/core/metrics.cc
    src/core/network_stack_registry.cc
    src/core/on_internal_error.cc
    src/core/posix.cc
    src/core/reactor.cc
    src/core/resource.cc
    src/core/sharded.cc
    src/core/scollectd.cc
    src/core/systemwide_memory_barrier.cc
    src/core/smp.cc
    src/core/sstring.cc
    src/core/thread.cc
    src/core/uname.cc
    src/core/io_queue.cc
    src/core/semaphore.cc
    src/core/condition-variable.cc

    src/detail/alloc_failure_injector.cc
    src/detail/backtrace.cc
    src/detail/conversions.cc
    src/detail/exceptions.cc
    src/detail/file.cc
    src/detail/log.cc
    src/detail/program-options.cc
    src/detail/read_first_line.cc
    src/detail/tmp_file.cc)

set(${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS
    -fvisibility=hidden
    -UNDEBUG
    -Wall
    -Werror
    -Wno-array-bounds # Disabled because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93437
    -Wno-error=deprecated-declarations
#    -Wno-unused-but-set-variable
    -fPIC)

if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang"))
    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS -fdeclspec)
endif()

if(UNIX AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    list(APPEND ${CURRENT_PROJECT_NAME}_HEADERS
         include/nil/actor/core/linux-aio.hh
         include/nil/actor/core/detail/reactor_backend_aio.hh)


    list(APPEND ${CURRENT_PROJECT_NAME}_SOURCES
         src/core/linux-aio.cc
         src/core/reactor_backend_aio.cc)
endif()

list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_INCLUDES
     $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
     $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
     $<BUILD_INTERFACE:${BUILD_WITH_GEN_BINARY_DIR}/include>
     $<INSTALL_INTERFACE:include>)

list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_INCLUDES
     ${CMAKE_CURRENT_SOURCE_DIR}/src
     ${BUILD_WITH_GEN_BINARY_DIR}/src

     ${Boost_INCLUDE_DIRS})

list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES
     ${Boost_LIBRARIES}
     ${c-ares_LIBRARIES}
     ${lz4_LIBRARIES}
     ${StdAtomic_LIBRARIES}
     ${Protobuf_LIBRARIES}

     fmt::fmt)

if(NOT (UNIX AND (CMAKE_SYSTEM_NAME STREQUAL "Linux")))
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES epoll-shim)
endif()

list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_LIBRARIES
     ${GnuTLS_LIBRARIES}
     ${yaml-cpp_LIBRARIES})

#if(BUILD_SHARED_LIBS)
#    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_LIBRARIES Threads::Threads)
#else()
list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES Threads::Threads ${CMAKE_DL_LIBS})
#endif()

list(APPEND ${CURRENT_PROJECT_NAME}_INTERFACE_LIBRARIES
     crypto3::block
     crypto3::hash)

set(BUILD_WITH_SANITIZE_MODES "Debug" "Sanitize")
if(BUILD_WITH_SANITIZE OR (CMAKE_BUILD_TYPE IN_LIST BUILD_WITH_SANITIZE_MODES))
    if(NOT Sanitizers_FOUND)
        message(FATAL_ERROR "Sanitizers not found!")
    endif()
    set(ACTOR_Sanitizers_OPTIONS ${Sanitizers_COMPILER_OPTIONS})
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES
         Sanitizers::address
         Sanitizers::undefined_behavior)
endif()

# We only need valgrind to find uninitialized memory uses, so disable
# the leak sanitizer.
# To test with valgrind run "ctest -T memcheck"
set(MEMORYCHECK_COMMAND_OPTIONS "--error-exitcode=1 --leak-check=no --trace-children=yes")
include(CTest)

#
# We want asserts enabled on all modes, but cmake defaults to passing
# -DNDEBUG in some modes. We add -UNDEBUG to our private options to
# reenable it. To force asserts off pass -DNDEBUG in
# ACTOR_CXX_FLAGS.
#
# To disable -Werror, pass -Wno-error to ACTOR_CXX_FLAGS.
#
# We disable _FORTIFY_SOURCE because it generates false positives with longjmp() (src/core/thread.cc)
#

list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS
     -U_FORTIFY_SOURCE)

list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS
     ACTOR_API_LEVEL=${BUILD_WITH_API_LEVEL})

if(BUILD_WITH_COMPRESS_DEBUG)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        # -gz doesn't imply -g, so it is safe to add it regardless of debug
        # info being enabled.
        list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS -gz)
    endif()
endif()

if(NOT BUILD_WITH_EXCEPTION_SCALABILITY_WORKAROUND)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS ACTOR_NO_EXCEPTION_HACK)
endif()

function(actor_supports_flag flag var)
    set(CMAKE_REQUIRED_FLAGS ${flag})
    check_cxx_source_compiles("int main() { return 0; }" result)
    set(${var} ${result} PARENT_SCOPE)
endfunction()

actor_supports_flag("-Wno-maybe-uninitialized -Werror" MaybeUninitialized_FOUND)
if(MaybeUninitialized_FOUND)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS
         # With std::experimental::optional it is easy to hit
         # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88897.  We disable
         # -Wmaybe-uninitialized in here since otherwise we would have to
         # disable it on many types used inside optional<>.
         -Wno-maybe-uninitialized)
endif()

if(BUILD_WITH_SSTRING)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS -DACTOR_SSTRING)
endif()

include(CheckFunctionExists)
check_function_exists(posix_fallocate ACTOR_HAS_POSIX_FALLOCATE)
if(ACTOR_HAS_POSIX_FALLOCATE)
    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS ACTOR_HAS_POSIX_FALLOCATE)
endif()

if(LinuxMembarrier_FOUND)
    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS
         ACTOR_HAS_MEMBARRIER)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES
         LinuxMembarrier::membarrier)
endif()

set(BUILD_WITH_ALLOC_FAILURE_INJECTION_MODES "Dev")
if(BUILD_WITH_ALLOC_FAILURE_INJECTION OR CMAKE_BUILD_TYPE IN_LIST
   BUILD_WITH_ALLOC_FAILURE_INJECTION_MODES)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS
         ACTOR_ENABLE_ALLOC_FAILURE_INJECTION)
endif()

if(BUILD_WITH_TASK_BACKTRACE)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS
         ACTOR_TASK_BACKTRACE)
endif()

if(BUILD_WITH_DEBUG_ALLOCATIONS)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS
         ACTOR_DEBUG_ALLOCATIONS)
endif()

if(BUILD_WITH_FIBER_SUPPORT)
    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS
         ACTOR_HAVE_ASAN_FIBER_SUPPORT)
endif()

if(BUILD_WITH_ALLOC_PAGE_SIZE)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS
         ACTOR_OVERRIDE_ALLOCATOR_PAGE_SIZE=${BUILD_WITH_ALLOC_PAGE_SIZE})
endif()

if(BUILD_WITH_CXX_FLAGS)
    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS ${BUILD_WITH_CXX_FLAGS})
endif()

# When using split dwarf --gdb-index is effectively required since
# otherwise gdb is just too slow. We also want to use split dwarf in
# as many compilation units as possible.  So while these flags don't
# have to be public, we don't expect anyone to want to build actor
# with them and some client code without.
if(BUILD_WITH_SPLIT_DWARF AND (NOT (CMAKE_BUILD_TYPE STREQUAL "Dev")))
    set(BUILD_WITH_SPLIT_DWARF_FLAG "-Wl,--gdb-index")
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES ${BUILD_WITH_SPLIT_DWARF_FLAG})
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS "-gsplit-dwarf")
endif()

if(BUILD_WITH_HEAP_PROFILING)
    set_property(SOURCE "src/core/memory.cc"
                 PROPERTY COMPILE_DEFINITIONS ACTOR_HEAPPROF)
endif()

if(BUILD_WITH_DPDK)
    #
    # The DPDK architecture needs to be transitively applied to consumers of Actor as well.
    #

    set(${CURRENT_PROJECT_NAME}_ARCH_FOR_native "native")
    set(${CURRENT_PROJECT_NAME}_ARCH_FOR_nhm "nehalem")
    set(${CURRENT_PROJECT_NAME}_ARCH_FOR_wsm "westmere")
    set(${CURRENT_PROJECT_NAME}_ARCH_FOR_snb "sandybridge")
    set(${CURRENT_PROJECT_NAME}_ARCH_FOR_ivb "ivybridge")
    set(${CURRENT_PROJECT_NAME}_ARCH_FOR_armv8a "armv8-a")
    set(${CURRENT_PROJECT_NAME}_ARCH ${${CURRENT_PROJECT_NAME}_ARCH_FOR_${BUILD_WITH_DPDK_MACHINE}})

    if(NOT ${CURRENT_PROJECT_NAME}_ARCH)
        message(FATAL_ERROR "Unrecognized DPDK machine identifier: ${BUILD_WITH_DPDK_MACHINE}")
    endif()

    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS -march=${${CURRENT_PROJECT_NAME}_ARCH})
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS ACTOR_HAVE_DPDK)

    # No pmd driver code will be pulled in without "--whole-archive". To
    # avoid exposing that to actor users, combine dpdk into a single
    # .o file.
    add_custom_command(OUTPUT actor-dpdk.o
                       COMMAND ld -r -o actor-dpdk.o --whole-archive ${dpdk_LIBRARIES}
                       --no-whole-archive)

    # This just provides the include path to cmake
    #    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES dpdk::dpdk)
endif()

if(BUILD_WITH_HWLOC)
    if(NOT hwloc_FOUND)
        message(FATAL_ERROR "`hwloc` support is enabled but it is not available!")
    endif()

    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS ACTOR_HAVE_HWLOC)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES hwloc::hwloc)
endif()

if(BUILD_WITH_LD_FLAGS)
    # In newer versions of CMake, there is `target_link_options`.
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LINK_OPTIONS ${BUILD_WITH_LD_FLAGS})
endif()

if(BUILD_WITH_NUMA AND NOT APPLE)
    if(NOT numactl_FOUND)
        message(FATAL_ERROR "NUMA support is enabled but `numactl` is not available!")
    endif()

    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS ACTOR_HAVE_NUMA)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES ${numactl_LIBRARIES})
endif()

if(lz4_HAVE_COMPRESS_DEFAULT)
    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS ACTOR_HAVE_LZ4_COMPRESS_DEFAULT)
endif()

actor_supports_flag("-Werror=unused-result" ErrorUnused_FOUND)
if(ErrorUnused_FOUND)
    if(BUILD_WITH_UNUSED_RESULT_ERROR)
        list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_OPTIONS -Werror=unused-result)
    else()
        list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_OPTIONS -Wno-error=unused-result)
    endif()
endif()

if((CMAKE_BUILD_TYPE STREQUAL "Debug") OR (CMAKE_BUILD_TYPE STREQUAL "Sanitize"))
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS
         ACTOR_DEBUG
         ACTOR_DEFAULT_ALLOCATOR
         ACTOR_SHUFFLE_TASK_QUEUE)
endif()

set(ACTOR_DEBUG_SHARED_PTR_MODES "Debug" "Sanitize")
if(BUILD_WITH_DEBUG_SHARED_PTR OR (CMAKE_BUILD_TYPE IN_LIST ACTOR_DEBUG_SHARED_PTR_MODES))
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS ACTOR_DEBUG_SHARED_PTR)
endif()

set(ACTOR_STACK_GUARD_MODES "Debug" "Sanitize" "Dev")
if(BUILD_WITH_STACK_GUARDS OR (CMAKE_BUILD_TYPE IN_LIST ACTOR_STACK_GUARD_MODES))
    # check for -fstack-clash-protection together with -Werror, because
    # otherwise clang can soft-fail (return 0 but emit a warning) instead.
    actor_supports_flag("-fstack-clash-protection -Werror" StackClashProtection_FOUND)
    if(StackClashProtection_FOUND)
        list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS -fstack-clash-protection)
    endif()
    list(APPEND ${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS ACTOR_THREAD_STACK_GUARDS)
endif()

if((CMAKE_BUILD_TYPE STREQUAL "Dev") OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS ACTOR_TYPE_ERASE_MORE)
endif()

if(UNIX AND NOT APPLE)
    list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES
         ${lksctp-tools_LIBRARIES}
         ${rt_LIBRARIES})
elseif(APPLE)

endif()

list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_LINK_OPTIONS "-fPIC")

add_library(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} STATIC
            ${${CURRENT_PROJECT_NAME}_HEADERS}
            ${${CURRENT_PROJECT_NAME}_SOURCES})

set_target_properties(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PROPERTIES
                      LINKER_LANGUAGE CXX
                      EXPORT_NAME ${CURRENT_PROJECT_NAME})

if(APPLE OR NOT ${CMAKE_TARGET_ARCHITECTURE} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
    set_target_properties(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PROPERTIES
                          XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
                          XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}")
endif()

target_compile_options(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                       PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS}
                       PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS}
                       ${${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_OPTIONS})

target_compile_definitions(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                           PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS}
                           PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS})

target_include_directories(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                           PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_INCLUDES}
                           PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_INCLUDES})

target_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                      PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_LIBRARIES} ${CMAKE_WORKSPACE_NAME}::network
                      PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_LIBRARIES}
                      INTERFACE ${${CURRENT_PROJECT_NAME}_INTERFACE_LIBRARIES})

target_link_options(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                    PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_LINK_OPTIONS})

if(BUILD_TESTS)
    set(testing_HEADERS
        include/nil/actor/testing/entry_point.hh
        include/nil/actor/testing/exchanger.hh
        include/nil/actor/testing/random.hh
        include/nil/actor/testing/actor_test.hh
        include/nil/actor/testing/test_case.hh
        include/nil/actor/testing/test_runner.hh
        include/nil/actor/testing/thread_test_case.hh)

    set(testing_SOURCES
        src/testing/entry_point.cc
        src/testing/random.cc
        src/testing/actor_test.cc
        src/testing/test_runner.cc)

    set(perf_testing_HEADERS
        include/nil/actor/testing/perf_tests.hh)

    set(perf_testing_SOURCES
        src/testing/random.cc
        perf/perf_tests.cc)

    add_library(${CMAKE_WORKSPACE_NAME}_testing
                ${testing_HEADERS}
                ${testing_SOURCES})

    set_target_properties(${CMAKE_WORKSPACE_NAME}_testing PROPERTIES
                          LINKER_LANGUAGE CXX
                          EXPORT_NAME testing)

    if(APPLE OR NOT ${CMAKE_TARGET_ARCHITECTURE} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        set_target_properties(${CMAKE_WORKSPACE_NAME}_testing PROPERTIES
                              XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
                              XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}")
    endif()

    target_compile_options(${CMAKE_WORKSPACE_NAME}_testing
                           PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS}
                           PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS}
                           ${${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_OPTIONS})

    target_compile_definitions(${CMAKE_WORKSPACE_NAME}_testing
                               PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS}
                               PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS})

    target_include_directories(${CMAKE_WORKSPACE_NAME}_testing
                               PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_INCLUDES}
                               PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_INCLUDES})

    target_link_libraries(${CMAKE_WORKSPACE_NAME}_testing PUBLIC
                          ${CMAKE_WORKSPACE_NAME}::core

                          ${Boost_LIBRARIES})

    target_link_options(${CMAKE_WORKSPACE_NAME}_testing PUBLIC ${testing_PUBLIC_LINK_OPTIONS})

    target_compile_definitions(${CMAKE_WORKSPACE_NAME}_testing PRIVATE BOOST_TEST_DYN_LINK BOOST_TEST_NO_MAIN)

    add_library(${CMAKE_WORKSPACE_NAME}_perf_testing
                ${perf_testing_HEADERS}
                ${perf_testing_SOURCES})

    target_compile_definitions(${CMAKE_WORKSPACE_NAME}_perf_testing PRIVATE BOOST_TEST_DYN_LINK BOOST_TEST_NO_MAIN)

    set_target_properties(${CMAKE_WORKSPACE_NAME}_perf_testing PROPERTIES
                          LINKER_LANGUAGE CXX
                          EXPORT_NAME perf_testing)

    if(APPLE OR NOT ${CMAKE_TARGET_ARCHITECTURE} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        set_target_properties(${CMAKE_WORKSPACE_NAME}_perf_testing PROPERTIES
                              XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
                              XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}")
    endif()

    target_compile_options(${CMAKE_WORKSPACE_NAME}_perf_testing
                           PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_OPTIONS}
                           PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS}
                           ${${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_OPTIONS})

    target_compile_definitions(${CMAKE_WORKSPACE_NAME}_perf_testing
                               PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_COMPILE_DEFINITIONS}
                               PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_COMPILE_DEFINITIONS})

    target_include_directories(${CMAKE_WORKSPACE_NAME}_perf_testing
                               PUBLIC ${${CURRENT_PROJECT_NAME}_PUBLIC_INCLUDES}
                               PRIVATE ${${CURRENT_PROJECT_NAME}_PRIVATE_INCLUDES})

    target_link_libraries(${CMAKE_WORKSPACE_NAME}_perf_testing PUBLIC
                          ${CMAKE_WORKSPACE_NAME}::core

                          Boost::unit_test_framework

                          ${Boost_LIBRARIES})

    target_link_options(${CMAKE_WORKSPACE_NAME}_perf_testing PUBLIC ${testing_PUBLIC_LINK_OPTIONS})

    cm_deploy(TARGETS ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
              ${CMAKE_WORKSPACE_NAME}_testing
              ${CMAKE_WORKSPACE_NAME}_perf_testing
              INCLUDE include
              NAMESPACE ${CMAKE_WORKSPACE_NAME}::)

    add_subdirectory(test)
else()
    cm_deploy(TARGETS ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
              INCLUDE include
              NAMESPACE ${CMAKE_WORKSPACE_NAME}::)
endif()
