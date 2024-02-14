function(add_tests)
    foreach(name ${ARGN})
        string(REPLACE "/" "_" test "${name}")

        add_unit_test(
            ${test}_test
            CATCH2
            FILES
            ${name}.cpp
            LIBRARIES
            warnings
            groov)
    endforeach()
endfunction()

add_tests(path read tree write)
add_subdirectory(fail)
