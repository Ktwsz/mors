project('mors-frontend', 'cpp')

includes = include_directories('main')

front_lib = static_library('mors-frontend', 'main/lib.cpp')

front_dep = declare_dependency(link_with: front_lib, include_directories: includes)
