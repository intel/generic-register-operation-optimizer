cmake_minimum_required(VERSION 3.21)

project(
    groov
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A generic register operation optimizer.")

include(cmake/get_cpm.cmake)
if(PROJECT_IS_TOP_LEVEL)
    cpmaddpackage("gh:intel/cicd-repo-infrastructure#dev")
else()
    cpmaddpackage("gh:intel/cicd-repo-infrastructure#3e2bef0")
endif()

add_versioned_package("gh:boostorg/mp11#boost-1.83.0")
add_versioned_package("gh:intel/cpp-baremetal-concurrency#0ddce52")
add_versioned_package("gh:intel/cpp-std-extensions#7e1cbc7")
add_versioned_package("gh:intel/cpp-baremetal-senders-and-receivers#22c8006")
add_versioned_package("gh:intel/safe-arithmetic#9ea549a")

find_package(Python3 COMPONENTS Interpreter)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

add_library(groov INTERFACE)
target_compile_features(groov INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
target_link_libraries_system(groov INTERFACE async boost_mp11 safe_arithmetic
                             stdx)
target_compile_options(
    groov
    INTERFACE
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},14>>:-Wno-gnu-string-literal-operator-template>
)

target_sources(
    groov
    INTERFACE FILE_SET
              groov
              TYPE
              HEADERS
              BASE_DIRS
              include
              FILES
              include/groov/attach_value.hpp
              include/groov/boost_extra.hpp
              include/groov/config.hpp
              include/groov/groov.hpp
              include/groov/identity.hpp
              include/groov/make_spec.hpp
              include/groov/mmio_bus.hpp
              include/groov/path.hpp
              include/groov/read.hpp
              include/groov/read_spec.hpp
              include/groov/resolve.hpp
              include/groov/value_path.hpp
              include/groov/write.hpp
              include/groov/write_spec.hpp)

if(PROJECT_IS_TOP_LEVEL)
    add_docs(docs)
    add_subdirectory(test)
    clang_tidy_interface(groov)
endif()
