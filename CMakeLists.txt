cmake_minimum_required(VERSION 3.5.0)

project(mors VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

add_executable(mors mors.cpp)

add_subdirectory(mors-parser)
add_subdirectory(ir)

add_dependencies(mors ir_python)

target_link_libraries(mors PUBLIC mors-parser 
                                  pybind11::pybind11
                                  ${Python3_LIBRARIES})

target_include_directories(mors PUBLIC "${PROJECT_BINARY_DIR}"
                                       "${PROJECT_SOURCE_DIR}/mors-parser"
                                       ${Python3_INCLUDE_DIRS})


