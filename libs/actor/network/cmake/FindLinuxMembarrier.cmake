find_path (LinuxMembarrier_INCLUDE_DIR
  NAMES linux/membarrier.h)

include (CheckCXXSourceCompiles)
file (READ ${CMAKE_CURRENT_LIST_DIR}/code_tests/LinuxMembarrier_test.cc _linuxmembarrier_test_code)
check_cxx_source_compiles ("${_linuxmembarrier_test_code}" LinuxMembarrier_FOUND)

if (LinuxMembarrier_FOUND)
  set (LinuxMembarrier_INCLUDE_DIRS ${LinuxMembarrier_INCLUDE_DIR})
endif ()

if (LinuxMembarrier_FOUND AND NOT (TARGET LinuxMembarrier::membarrier))
  add_library (LinuxMembarrier::membarrier INTERFACE IMPORTED)

  set_target_properties (LinuxMembarrier::membarrier
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${LinuxMembarrier_INCLUDE_DIRS})
endif ()
