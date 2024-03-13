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

add_tests(
    config
    identity
    mmio_bus
    path
    read
    read_spec
    value_path
    write
    write_spec)
add_subdirectory(fail)

add_unit_test(
    mmio_bus_pbt_test
    PYTEST
    FILES
    mmio_bus_pbt.py
    INCLUDE_FILES
    mmio_bus_pbt.hpp
    LIBRARIES
    groov)
