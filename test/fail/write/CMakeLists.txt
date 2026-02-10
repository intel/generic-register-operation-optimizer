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

add_fail_tests(incur_rmw_on_wo_bits incur_rmw_on_wo_field
               write_to_ro_field_direct write_to_ro_register_direct)
