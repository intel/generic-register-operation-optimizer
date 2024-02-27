# ~~~
# Create a header-only library from an SVD register definition file.
#
# add_groov_reg_lib(
#     groov_reg_test
#     INPUT_FILE "${CMAKE_SOURCE_DIR}/regs/rp2040.svd"
#     OUTPUT_DIR "${CMAKE_BINARY_DIR}/regs"
#     PERIPHERALS adc rtc
# )
# ~~~

function(add_groov_reg_lib target_name)
    message(ERROR "Python3 not found, `add_groov_reg_lib` is not available.")
endfunction()

if(NOT Python3_FOUND)
    return()
endif()

function(add_groov_reg_lib target_name)
    set(oneValueArgs INPUT_FILE OUTPUT_DIR)
    set(multiValueArgs PERIPHERALS)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(python_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/regs2groov.py")

    list(TRANSFORM ARG_PERIPHERALS APPEND ".hpp" OUTPUT_VARIABLE
                                                 register_headers)

    add_custom_command(
        COMMAND_EXPAND_LISTS
        OUTPUT ${register_headers}
        COMMAND
            ${Python3_EXECUTABLE} ${python_script} --input ${ARG_INPUT_FILE}
            --output ${ARG_OUTPUT_DIR} --peripherals ${ARG_PERIPHERALS} --naming
            lower
        DEPENDS ${python_script} ${ARG_INPUT_FILE}
        COMMENT
            "Generating groov register header files (${register_headers}) from ${ARG_INPUT_FILE}."
    )

    add_custom_target(${target_name}_gen DEPENDS ${register_headers})
    add_library(${target_name} INTERFACE)
    add_dependencies(${target_name} ${target_name}_gen)
    target_include_directories(${target_name} INTERFACE ${ARG_OUTPUT_DIR})
endfunction()
