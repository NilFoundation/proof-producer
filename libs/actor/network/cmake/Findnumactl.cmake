find_path (numactl_INCLUDE_DIR
  NAMES numaif.h)

find_library (numactl_LIBRARY
  NAMES numa)

mark_as_advanced (
  numactl_INCLUDE_DIR
  numactl_LIBRARY)

include (FindPackageHandleStandardArgs)

find_package_handle_standard_args (numactl
  REQUIRED_VARS
    numactl_LIBRARY
    numactl_INCLUDE_DIR)

set (numactl_INCLUDE_DIRS ${numactl_INCLUDE_DIR})
set (numactl_LIBRARIES ${numactl_LIBRARY})

if (numactl_FOUND AND NOT (TARGET numactl::numactl))
  add_library (numactl::numactl UNKNOWN IMPORTED)

  set_target_properties (numactl::numactl
    PROPERTIES
      IMPORTED_LOCATION ${numactl_LIBRARIES}
      INTERFACE_INCLUDE_DIRECTORIES ${numactl_INCLUDE_DIRS})
endif ()
