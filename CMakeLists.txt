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
add_versioned_package("gh:intel/cpp-std-extensions#f67b432")
add_versioned_package("gh:intel/cpp-baremetal-senders-and-receivers#eb4239f")
add_versioned_package("gh:intel/safe-arithmetic#9ea549a")

find_package(Python3 COMPONENTS Interpreter)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

add_library(groov INTERFACE)
target_include_directories(groov INTERFACE include)
target_compile_features(groov INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
target_link_libraries_system(groov INTERFACE async boost_mp11 safe_arithmetic
                             stdx)

if(PROJECT_IS_TOP_LEVEL)
    add_docs(docs)
    add_subdirectory(test)
    clang_tidy_interface(groov)
endif()

add_subdirectory(tools)
