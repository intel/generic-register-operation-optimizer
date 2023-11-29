function(add_fail_tests)
    foreach(name ${ARGN})
        add_compile_fail_test("${name}.cpp" LIBRARIES groov)
    endforeach()
endfunction()

add_fail_tests(identifier_path_invalid identifier_path_too_long)
