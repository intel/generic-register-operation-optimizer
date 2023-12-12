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
    read_invalid_path)
