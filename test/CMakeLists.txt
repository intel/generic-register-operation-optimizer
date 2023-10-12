function(add_test_suites)
    foreach(test_file ${ARGN})
        string(REPLACE "/" "_" test_name "${test_file}")

        add_unit_test(
            ${test_name}
            GTEST
            FILES
            ${test_file}
            INCLUDE_DIRECTORIES
            ${CMAKE_SOURCE_DIR}/test/
            LIBRARIES
            groov)
    endforeach()
endfunction()

add_test_suites(object.cpp group.cpp identifier.cpp)
