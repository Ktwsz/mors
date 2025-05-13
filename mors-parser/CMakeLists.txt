include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

ExternalProject_Add(libminizinc
    GIT_REPOSITORY https://github.com/MiniZinc/libminizinc.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
)

include_directories(${EXTERNAL_INSTALL_LOCATION}/include)

add_library(mors-parser STATIC lib.cpp)

add_dependencies(mors-parser libminizinc)
target_link_libraries(mors-parser ${EXTERNAL_INSTALL_LOCATION}/lib/libmzn.a)
