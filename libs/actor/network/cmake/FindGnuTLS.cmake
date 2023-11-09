find_package (PkgConfig REQUIRED)

pkg_search_module (GnuTLS_PC gnutls)

find_library (GnuTLS_LIBRARY
  NAMES gnutls
  HINTS
    ${GnuTLS_PC_LIBDIR}
    ${GnuTLS_PC_LIBRARY_DIRS})

find_path (GnuTLS_INCLUDE_DIR
  NAMES gnutls/gnutls.h
  HINTS
    ${GnuTLS_PC_INCLUDEDIR}
    ${GnuTLS_PC_INCLUDE_DIRS})

mark_as_advanced (
  GnuTLS_LIBRARY
  GnuTLS_INCLUDE_DIR)

include (FindPackageHandleStandardArgs)

find_package_handle_standard_args (GnuTLS
  REQUIRED_VARS
    GnuTLS_LIBRARY
    GnuTLS_INCLUDE_DIR
  VERSION_VAR GnuTLS_PC_VERSION)

set (GnuTLS_LIBRARIES ${GnuTLS_LIBRARY})
set (GnuTLS_INCLUDE_DIRS ${GnuTLS_INCLUDE_DIR})

if (GnuTLS_FOUND AND NOT (TARGET GnuTLS::gnutls))
  add_library (GnuTLS::gnutls UNKNOWN IMPORTED)

  set_target_properties (GnuTLS::gnutls
    PROPERTIES
      IMPORTED_LOCATION ${GnuTLS_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${GnuTLS_INCLUDE_DIRS})
endif ()
