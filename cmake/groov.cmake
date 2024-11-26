function(generate_register_group target)
    set(oneValueArgs
        INPUT
        PARSER_MODULES
        PARSE_FN
        BUS
        GROUP
        OUTPUT
        NAMESPACE)
    set(multiValueArgs INCLUDES LIBRARIES REGISTERS)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(python_script ${CMAKE_SOURCE_DIR}/tools/regs2groov.py)

    set(base_dir ${CMAKE_CURRENT_BINARY_DIR})
    if(NOT ARG_GROUP)
        set(ARG_GROUP ${target})
    endif()
    if(NOT ARG_OUTPUT)
        set(ARG_OUTPUT "${base_dir}/${target}.hpp")
    else()
        set(ARG_OUTPUT "${base_dir}/${ARG_OUTPUT}")
    endif()

    add_custom_command(
        COMMAND_EXPAND_LISTS
        OUTPUT ${ARG_OUTPUT}
        COMMAND
            ${Python3_EXECUTABLE} ${python_script} --bus "\"${ARG_BUS}\""
            --group ${ARG_GROUP} --includes ${ARG_INCLUDES} --input ${ARG_INPUT}
            --namespace ${ARG_NAMESPACE} --naming upper --output ${ARG_OUTPUT}
            --parse-fn ${ARG_PARSE_FN} --parser-modules ${ARG_PARSER_MODULES}
            --registers ${ARG_REGISTERS}
        DEPENDS ${python_script} ${ARG_INPUT}
        COMMENT
            "Generating groov register group (${ARG_GROUP}) header file ${ARG_OUTPUT} from ${ARG_INPUT}."
    )
    add_custom_target(${target}_gen DEPENDS ${ARG_OUTPUT})

    add_library(${target} INTERFACE)
    target_link_libraries_system(${target} INTERFACE ${ARG_LIBRARIES})
    add_dependencies(${target} ${target}_gen)
    target_sources(
        ${target}
        INTERFACE FILE_SET
                  ${target}
                  TYPE
                  HEADERS
                  BASE_DIRS
                  ${base_dir}
                  FILES
                  ${ARG_OUTPUT})
endfunction()
