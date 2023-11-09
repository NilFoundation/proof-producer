set (_rt_test_source ${CMAKE_CURRENT_LIST_DIR}/code_tests/rt_test.cc)

# Try to compile without the library first.
try_compile (rt_NO_EXPLICIT_LINK
  ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${_rt_test_source})

if (rt_NO_EXPLICIT_LINK)
  set (rt_FOUND yes)
else ()
  # The `rt` library is required.

  try_compile (_rt_test
    ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES ${_rt_test_source}
    LINK_LIBRARIES rt)

  if (_rt_test)
    set (rt_LIBRARY_NAME rt)
    set (rt_LIBRARIES -l${rt_LIBRARY_NAME})
  endif ()

  include (FindPackageHandleStandardArgs)

  find_package_handle_standard_args (rt
    REQUIRED_VARS rt_LIBRARIES)
endif ()

if (rt_FOUND AND NOT (TARGET rt::rt))
  add_library (rt::rt INTERFACE IMPORTED)

  set_target_properties (rt::rt
    PROPERTIES
      INTERFACE_LINK_LIBRARIES "${rt_LIBRARIES}")
endif ()
