cmake_minimum_required(VERSION 3.21)

project(
    groov
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A generic register operation optimizer.")

include(cmake/get_cpm.cmake)
cpmaddpackage("gh:intel/cicd-repo-infrastructure#main")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

add_library(groov INTERFACE)
target_include_directories(groov INTERFACE include)
target_compile_features(groov INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

if(PROJECT_IS_TOP_LEVEL)
    add_docs(docs)
    add_subdirectory(test)
    clang_tidy_interface(groov)
endif()
