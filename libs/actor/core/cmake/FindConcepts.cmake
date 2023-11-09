include(CheckCXXSourceCompiles)
file(READ ${CMAKE_CURRENT_LIST_DIR}/code_tests/Concepts_test.cc _concepts_test_code)
set(CMAKE_REQUIRED_FLAGS -fconcepts)
check_cxx_source_compiles("${_concepts_test_code}" Concepts_FOUND)
set(CMAKE_REQUIRED_FLAGS "")

if(Concepts_FOUND AND NOT (TARGET Concepts::concepts))
    set(CMAKE_REQUIRED_FLAGS -fconcepts)
    add_library(Concepts::concepts INTERFACE IMPORTED)

    set_target_properties(Concepts::concepts
                          PROPERTIES
                          INTERFACE_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CXX>:-fconcepts>)
endif()
