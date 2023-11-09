find_program (
  ragel_RAGEL_EXECUTABLE
  ragel)

mark_as_advanced (ragel_RAGEL_EXECUTABLE)

set (_ragel_version_pattern "[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?")

if (ragel_RAGEL_EXECUTABLE)
  set (ragel_FOUND ON)

  exec_program (${ragel_RAGEL_EXECUTABLE}
    ARGS -v
    OUTPUT_VARIABLE _ragel_version_output)

  if (${_ragel_version_output} MATCHES "version (${_ragel_version_pattern})")
    set (ragel_VERSION ${CMAKE_MATCH_1})
  endif ()
endif ()

find_package_handle_standard_args (ragel
  REQUIRED_VARS ragel_RAGEL_EXECUTABLE
  VERSION_VAR ragel_VERSION)
