find_package(pybind11 REQUIRED)

pybind11_add_module(ir_python ir.cpp)

target_include_directories(ir_python PUBLIC "${PROJECT_SOURCE_DIR}/mors-parser")
target_link_directories(ir_python PUBLIC "${PROJECT_SOURCE_DIR}/mors-parser")

set_target_properties(ir_python
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

