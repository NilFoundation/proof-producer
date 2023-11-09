find_package (PkgConfig REQUIRED)

pkg_search_module (lz4_PC liblz4)

find_library (lz4_LIBRARY
  NAMES lz4
  HINTS
    ${lz4_PC_LIBDIR}
    ${lz4_PC_LIBRARY_DIRS})

find_path (lz4_INCLUDE_DIR
  NAMES lz4.h
  HINTS
    ${lz4_PC_INCLUDEDIR}
    ${lz4_PC_INCLUDEDIRS})

mark_as_advanced (
  lz4_LIBRARY
  lz4_INCLUDE_DIR)

include (FindPackageHandleStandardArgs)

find_package_handle_standard_args (lz4
  REQUIRED_VARS
    lz4_LIBRARY
    lz4_INCLUDE_DIR
  VERSION_VAR lz4_PC_VERSION)

set (lz4_LIBRARIES ${lz4_LIBRARY})
set (lz4_INCLUDE_DIRS ${lz4_INCLUDE_DIR})

if (lz4_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES ${lz4_LIBRARY})
  include (CheckSymbolExists)

  check_symbol_exists (LZ4_compress_default
    ${lz4_INCLUDE_DIR}/lz4.h
    lz4_HAVE_COMPRESS_DEFAULT)
endif ()

if (lz4_FOUND AND NOT (TARGET lz4::lz4))
  add_library (lz4::lz4 UNKNOWN IMPORTED)

  set_target_properties (lz4::lz4
    PROPERTIES
      IMPORTED_LOCATION ${lz4_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${lz4_INCLUDE_DIRS})
endif ()
