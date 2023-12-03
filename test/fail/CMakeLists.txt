function(add_fail_tests)
    foreach(name ${ARGN})
        add_compile_fail_test("${name}.cpp" LIBRARIES groov)
    endforeach()
endfunction()

add_fail_tests(
    path_invalid
    path_too_long
    read_ambiguous_path
    read_ambiguous_multireg_path
    read_invalid_path
    read_invalid_multifield_path
    read_invalid_multireg_path)
