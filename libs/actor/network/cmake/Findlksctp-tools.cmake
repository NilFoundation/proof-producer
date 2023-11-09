find_library (lksctp-tools_LIBRARY
  NAMES sctp)

find_path (lksctp-tools_INCLUDE_DIR
  NAMES netinet/sctp.h)

mark_as_advanced (
  lksctp-tools_LIBRARY
  lksctp-tools_INCLUDE_DIR)

include (FindPackageHandleStandardArgs)

find_package_handle_standard_args (lksctp-tools
  REQUIRED_VARS
    lksctp-tools_LIBRARY
    lksctp-tools_INCLUDE_DIR)

set (lksctp-tools_LIBRARIES ${lksctp-tools_LIBRARY})
set (lksctp-tools_INCLUDE_DIRS ${lksctp-tools_INCLUDE_DIR})

if (lksctp-tools_FOUND AND NOT (TARGET lksctp-tools::lksctp-tools))
  add_library (lksctp-tools::lksctp-tools UNKNOWN IMPORTED)

  set_target_properties (lksctp-tools::lksctp-tools
    PROPERTIES
      IMPORTED_LOCATION ${lksctp-tools_LIBRARIES}
      INTERFACE_INCLUDE_DIRECTORIES ${lksctp-tools_INCLUDE_DIRS})
endif ()

mark_as_advanced (
  lksctp-tools_INCLUDE_DIR
  lksctp-tools_LIBRARY)
