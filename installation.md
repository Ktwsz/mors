### Requirements:
    C++ compiler, python, CMake, Conan

### Install minizinc's standard library and add OR-Tools bindings

### Setup conan
1. Profile detect

    `conan profile detect --force`
2. In your profile - `.conan2/profiles/default`
    * Change compiler.cppstd to c++20
    * Change compiler.libcxx=libstdc++11


### Build commands
1. `conan install . --build=missing -s build_type=Debug`
2. `cd build`
3. `cmake .. --preset conan-debug`
4. `cmake --build Debug`


### Run commands
`./build/Debug/mors models/m_cumulative.mzn models/m_cumulative.dzn`
