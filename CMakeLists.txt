cmake_minimum_required(VERSION 3.5.0)

project(mors VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_executable(mors mors.cpp)

add_subdirectory(mors-parser)
target_link_libraries(mors PUBLIC mors-parser)
target_include_directories(mors PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/mors-parser")


