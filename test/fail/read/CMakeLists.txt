if(${CMAKE_CXX_STANDARD} LESS 20)
    return()
endif()
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_VERSION}
                                                 VERSION_LESS 15)
    return()
endif()
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION}
                                               VERSION_LESS 13.2)
    return()
endif()

add_fail_tests(
    read_from_wo_bits_indirect read_from_wo_field_indirect_by_field
    read_from_wo_field_indirect_by_register read_from_wo_field_direct
    read_from_wo_register_direct)
