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
    read_only_field_without_identity
    sync_read_nontrivial
    sync_write_nontrivial
    sync_write_nontrivial_nodiscard
    write_spec_literal_index)

target_compile_options(EXPECT_FAIL.sync_write_nontrivial_nodiscard.cpp
                       PRIVATE -Werror)

add_subdirectory(read)
add_subdirectory(write)
