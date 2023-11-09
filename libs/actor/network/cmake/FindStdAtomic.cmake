function (_stdatomic_can_link var)
  include (CheckCXXSourceCompiles)
  set (test_code "int main() {}")
  set (CMAKE_REQUIRED_LIBRARIES -latomic)
  check_cxx_source_compiles ("${test_code}" ${var})
endfunction ()

_stdatomic_can_link (StdAtomic_EXPLICIT_LINK)

#
# If linking against `-latomic` is successful, then do it unconditionally.
#

if (StdAtomic_EXPLICIT_LINK)
  set (StdAtomic_LIBRARY_NAME atomic)
  set (StdAtomic_LIBRARIES -l${StdAtomic_LIBRARY_NAME})
  include (FindPackageHandleStandardArgs)

  find_package_handle_standard_args (StdAtomic
    REQUIRED_VARS StdAtomic_LIBRARIES)
endif ()

if (NOT (TARGET StdAtomic::atomic))
  add_library (StdAtomic::atomic INTERFACE IMPORTED)

  set_target_properties (StdAtomic::atomic
    PROPERTIES
      INTERFACE_LINK_LIBRARIES "${StdAtomic_LIBRARIES}")
endif ()
