include(ExternalProject)

ExternalProject_Add(libminizinc
    GIT_REPOSITORY https://github.com/Ktwsz/libminizinc.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}/minizinc
)

add_library(mors-parser STATIC lib.cpp 
                               ast.cpp
                               ast_printer.cpp 
                               parser_opts.cpp 
                               parsing_errors.cpp
                               transformer.cpp
                               utils.cpp)

add_dependencies(mors-parser libminizinc clipp)
target_include_directories(mors-parser PUBLIC "${EXTERNAL_INSTALL_LOCATION}/minizinc/include" "${EXTERNAL_INSTALL_LOCATION}/clipp/include")
                                       
target_link_libraries(mors-parser ${EXTERNAL_INSTALL_LOCATION}/minizinc/lib/libmzn.a)

target_compile_options(mors-parser PRIVATE -Wall -Wextra -Wpedantic) # TODO: fix compilations errors -Werror
