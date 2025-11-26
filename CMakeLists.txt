include(ExternalProject)
cmake_minimum_required(VERSION 3.5.0)

project(mors VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)
ExternalProject_Add(clipp
    GIT_REPOSITORY https://github.com/Ktwsz/clipp.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}/clipp
)

add_executable(mors mors.cpp)

add_subdirectory(mors-parser)
add_subdirectory(ir)

add_dependencies(mors ir_python clipp)

target_link_libraries(mors PUBLIC mors-parser 
                                  ${Python3_LIBRARIES}
                                  pybind11::pybind11)

target_include_directories(mors PUBLIC "${PROJECT_SOURCE_DIR}/mors-parser"
                                       ${Python3_INCLUDE_DIRS}
                                       "${EXTERNAL_INSTALL_LOCATION}/clipp/include")

target_compile_options(mors PRIVATE -Wall -Wextra -Wpedantic -Werror)


