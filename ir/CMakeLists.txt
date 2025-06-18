find_package(pybind11 REQUIRED)

pybind11_add_module(ir_python ir.cpp)

set_target_properties(ir_python
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

