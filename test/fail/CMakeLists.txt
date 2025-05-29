function(add_fail_tests)
    foreach(name ${ARGN})
        add_compile_fail_test("${name}.cpp" LIBRARIES groov)
    endforeach()
endfunction()

add_fail_tests(
    group_duplicate_path
    group_redundant_path
    group_unresolvable_path
    path_ambiguous
    path_mismatch
    path_too_long
    read_ambiguous_path
    read_invalid_path
    read_only_field_without_identity)

function(add_formatted_error_tests)
    add_fail_tests(write_read_only_field)
endfunction()

if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 20)
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"
       AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 15)
        add_formatted_error_tests()
    endif()
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION}
                                                   VERSION_GREATER_EQUAL 13.2)
        add_formatted_error_tests()
    endif()
endif()
