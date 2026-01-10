pybind11_add_module(mors_ir ir.cpp)

target_include_directories(mors_ir PUBLIC "${PROJECT_SOURCE_DIR}/mors-parser")
target_link_directories(mors_ir PUBLIC "${PROJECT_SOURCE_DIR}/mors-parser")

set_target_properties(mors_ir
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

