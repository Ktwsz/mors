include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

ExternalProject_Add(libminizinc
    GIT_REPOSITORY https://github.com/Ktwsz/libminizinc.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
)

find_package(fmt REQUIRED)
find_package(clipp REQUIRED)

include_directories(${EXTERNAL_INSTALL_LOCATION}/include)

add_library(mors-parser STATIC lib.cpp 
                               ast_printer.cpp 
                               parser_opts.cpp 
                               parsing_errors.cpp)

add_dependencies(mors-parser libminizinc)
target_include_directories(mors-parser PUBLIC "${PROJECT_SOURCE_DIR}/ir")
target_link_libraries(mors-parser ${EXTERNAL_INSTALL_LOCATION}/lib/libmzn.a
                                  fmt::fmt
                                  clipp::clipp)

target_compile_options(mors-parser PRIVATE -Wall -Wextra -Wpedantic) # TODO: fix compilations errors -Werror
